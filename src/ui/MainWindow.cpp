#include "ui/MainWindow.h"

#include "shell/ShellActions.h"
#include "shell/ShellFileOperations.h"
#include "ui/RenameDialog.h"

#include <utility>

#include <windowsx.h>

namespace finderx {

namespace {

constexpr wchar_t kWindowClassName[] = L"FinderXMainWindow";
constexpr UINT kCommandOpen = 1001;
constexpr UINT kCommandReveal = 1002;
constexpr UINT kCommandCopyPath = 1003;
constexpr UINT kCommandRefresh = 1004;
constexpr UINT kCommandRename = 1005;
constexpr UINT kCommandCopy = 1006;
constexpr UINT kCommandCut = 1007;
constexpr UINT kCommandPaste = 1008;
constexpr UINT kCommandMoveToTrash = 1009;

bool containsPoint(const D2D1_RECT_F& rect, POINT point) {
    return static_cast<float>(point.x) >= rect.left && static_cast<float>(point.x) <= rect.right
        && static_cast<float>(point.y) >= rect.top && static_cast<float>(point.y) <= rect.bottom;
}

}

bool MainWindow::create(HINSTANCE instance, int showCommand) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = MainWindow::WndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    hwnd_ = CreateWindowExW(
        0,
        kWindowClassName,
        L"FinderX",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1188,
        768,
        nullptr,
        nullptr,
        instance,
        this);

    if (!hwnd_) {
        return false;
    }

    initializeFileTree();
    ShowWindow(hwnd_, showCommand);
    SetFocus(hwnd_);
    UpdateWindow(hwnd_);
    return true;
}

HWND MainWindow::hwnd() const {
    return hwnd_;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = nullptr;

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        window = static_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd_ = hwnd;
    } else {
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->handleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        SetFocus(hwnd_);
        return 0;
    case WM_PAINT:
        paint();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        render_.resize(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        SetFocus(hwnd_);
        const LayoutRects rects = currentLayout();
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));

        const ChromeHitResult chromeHit = chrome_.hitTest(x, y, rects, chromeState_);
        switch (chromeHit.kind) {
        case ChromeHitKind::Back:
            goBack();
            return 0;
        case ChromeHitKind::Forward:
            goForward();
            return 0;
        case ChromeHitKind::SidebarItem:
            if (chromeHit.sidebarIndex < chromeState_.sidebarItems.size()) {
                navigateToDirectory(chromeState_.sidebarItems[chromeHit.sidebarIndex].path, HistoryMode::Push);
            }
            return 0;
        case ChromeHitKind::None:
            break;
        }

        const ListInteractionResult result = listView_.onMouseDown(x, y, rects.list);
        loadChildrenIfNeeded(result.expandedFolder);
        activateNode(result.activatedNode);
        if (result.changed) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_RBUTTONDOWN: {
        SetFocus(hwnd_);
        const POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        POINT screenPoint = clientPoint;
        ClientToScreen(hwnd_, &screenPoint);
        showContextMenu(clientPoint, screenPoint);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        const LayoutRects rects = currentLayout();
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd_, &point);
        const float x = static_cast<float>(point.x);
        const float y = static_cast<float>(point.y);
        if (x >= rects.list.left && x <= rects.list.right && y >= rects.list.top && y <= rects.list.bottom
            && listView_.onWheel(GET_WHEEL_DELTA_WPARAM(wParam))) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_COMMAND:
        handleCommand(wParam);
        return 0;
    case WM_KEYDOWN: {
        contextNode_ = kInvalidNodeId;
        const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

        if (wParam == VK_F5 || (wParam == L'R' && controlDown)) {
            refreshCurrentDirectory();
            return 0;
        }

        if (wParam == VK_F2) {
            renameContextNode();
            return 0;
        }

        if (wParam == VK_DELETE) {
            moveContextNodeToTrash();
            return 0;
        }

        if (wParam == L'C' && controlDown) {
            copyContextNode();
            return 0;
        }

        if (wParam == L'X' && controlDown) {
            cutContextNode();
            return 0;
        }

        if (wParam == L'V' && controlDown) {
            pasteIntoCurrentDirectory();
            return 0;
        }

        if (wParam == VK_BACK) {
            goBack();
            return 0;
        }

        const ListInteractionResult result = listView_.onKeyDown(wParam);
        loadChildrenIfNeeded(result.expandedFolder);
        activateNode(result.activatedNode);
        if (result.changed) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

LayoutRects MainWindow::currentLayout() const {
    RECT client{};
    if (!hwnd_ || !GetClientRect(hwnd_, &client)) {
        return chrome_.layout(0.0f, 0.0f);
    }

    const float width = static_cast<float>(client.right - client.left);
    const float height = static_cast<float>(client.bottom - client.top);
    return chrome_.layout(width, height);
}

void MainWindow::initializeFileTree() {
    homePath_ = defaultHomeDirectory();
    navigateToDirectory(homePath_, HistoryMode::Initial);
}

bool MainWindow::navigateToDirectory(std::wstring path, HistoryMode mode) {
    if (path.empty()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(path);
    if (!result.ok()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    std::wstring rootName = fileNameFromPath(path);
    if (rootName.empty()) {
        rootName = path;
    }

    tree_ = FileTree(path, std::move(rootName));
    tree_.replaceChildren(tree_.rootId(), std::move(result.children));
    listView_ = FinderListView(&tree_);

    switch (mode) {
    case HistoryMode::Initial:
    case HistoryMode::Replace:
        history_.setInitialPath(std::move(path));
        break;
    case HistoryMode::Push:
        history_.navigateTo(std::move(path));
        break;
    case HistoryMode::BackForward:
        break;
    }

    chromeState_.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

void MainWindow::activateNode(NodeId nodeId) {
    if (nodeId == kInvalidNodeId || nodeId >= tree_.nodes().size()) {
        return;
    }

    const FileNode& node = tree_.node(nodeId);
    if (node.kind == FileKind::Folder) {
        navigateToDirectory(node.path, HistoryMode::Push);
    } else {
        openFile(node.path);
    }
}

void MainWindow::openFile(const std::wstring& path) {
    if (!shell::openPath(hwnd_, path)) {
        setStatusText(L"Cannot open file");
    }
}

void MainWindow::showContextMenu(POINT clientPoint, POINT screenPoint) {
    const LayoutRects rects = currentLayout();
    if (!containsPoint(rects.list, clientPoint)) {
        return;
    }

    contextNode_ = listView_.nodeAtPoint(
        static_cast<float>(clientPoint.x),
        static_cast<float>(clientPoint.y),
        rects.list);

    const bool hasTarget = contextNode_ != kInvalidNodeId;
    if (hasTarget && listView_.selectNode(contextNode_)) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    if (hasTarget) {
        AppendMenuW(menu, MF_STRING, kCommandOpen, L"Open");
        AppendMenuW(menu, MF_STRING, kCommandRename, L"Rename");
        AppendMenuW(menu, MF_STRING, kCommandCopy, L"Copy");
        AppendMenuW(menu, MF_STRING, kCommandCut, L"Cut");
        if (fileOperationState_.hasPendingOperation()) {
            AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kCommandReveal, L"Show in Explorer");
        AppendMenuW(menu, MF_STRING, kCommandCopyPath, L"Copy Path");
        AppendMenuW(menu, MF_STRING, kCommandMoveToTrash, L"Move to Trash");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    } else if (fileOperationState_.hasPendingOperation()) {
        AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(menu, MF_STRING, kCommandRefresh, L"Refresh");

    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

void MainWindow::handleCommand(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case kCommandOpen:
        openContextNode();
        break;
    case kCommandRename:
        renameContextNode();
        break;
    case kCommandCopy:
        copyContextNode();
        break;
    case kCommandCut:
        cutContextNode();
        break;
    case kCommandPaste:
        pasteIntoCurrentDirectory();
        break;
    case kCommandMoveToTrash:
        moveContextNodeToTrash();
        break;
    case kCommandReveal:
        revealContextNode();
        break;
    case kCommandCopyPath:
        copyContextNodePath();
        break;
    case kCommandRefresh:
        refreshCurrentDirectory();
        break;
    default:
        break;
    }
}

NodeId MainWindow::commandTargetNode() const {
    if (contextNode_ != kInvalidNodeId && contextNode_ < tree_.nodes().size()) {
        return contextNode_;
    }

    return listView_.selectedNode();
}

void MainWindow::openContextNode() {
    if (contextNode_ == kInvalidNodeId) {
        return;
    }
    activateNode(contextNode_);
}

void MainWindow::renameContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    const FileNode& node = tree_.node(target);
    std::wstring newName;
    if (!ui::promptForRename(hwnd_, node.name, newName)) {
        return;
    }

    const shell::FileOperationResult result = shell::renamePath(hwnd_, node.path, newName);
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot rename item" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting(result.resultingPath);
}

void MainWindow::moveContextNodeToTrash() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    const shell::FileOperationResult result = shell::moveToTrash(hwnd_, tree_.node(target).path);
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot move item to trash" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting({});
}

void MainWindow::copyContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    fileOperationState_.setCopy(tree_.node(target).path);
}

void MainWindow::cutContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    fileOperationState_.setCut(tree_.node(target).path);
}

void MainWindow::pasteIntoCurrentDirectory() {
    if (!fileOperationState_.hasPendingOperation()) {
        return;
    }

    const std::wstring currentPath = history_.currentPath();
    if (currentPath.empty()) {
        setStatusText(L"Cannot paste here");
        return;
    }

    shell::FileOperationResult result;
    if (fileOperationState_.kind() == ui::PendingFileOperationKind::Copy) {
        result = shell::copyToDirectory(hwnd_, fileOperationState_.sourcePath(), currentPath);
    } else {
        result = shell::moveToDirectory(hwnd_, fileOperationState_.sourcePath(), currentPath);
    }

    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot paste here" : result.message);
        return;
    }

    fileOperationState_.markPasteSucceeded();
    refreshCurrentDirectory();
}

void MainWindow::revealContextNode() {
    if (contextNode_ == kInvalidNodeId || contextNode_ >= tree_.nodes().size()) {
        return;
    }

    if (!shell::revealInExplorer(hwnd_, tree_.node(contextNode_).path)) {
        setStatusText(L"Cannot show in Explorer");
    }
}

void MainWindow::copyContextNodePath() {
    if (contextNode_ == kInvalidNodeId || contextNode_ >= tree_.nodes().size()) {
        return;
    }

    if (!shell::copyPathToClipboard(hwnd_, tree_.node(contextNode_).path)) {
        setStatusText(L"Cannot copy path");
    }
}

bool MainWindow::refreshCurrentDirectory() {
    return refreshCurrentDirectorySelecting(selectedNodePath());
}

bool MainWindow::refreshCurrentDirectorySelecting(const std::wstring& selectedPath) {
    const std::wstring currentPath = history_.currentPath();
    if (currentPath.empty()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(currentPath);
    if (!result.ok()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    std::wstring rootName = fileNameFromPath(currentPath);
    if (rootName.empty()) {
        rootName = currentPath;
    }

    tree_ = FileTree(currentPath, std::move(rootName));
    tree_.replaceChildren(tree_.rootId(), std::move(result.children));
    listView_ = FinderListView(&tree_);
    selectNodeByPath(selectedPath);
    contextNode_ = kInvalidNodeId;
    chromeState_.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

std::wstring MainWindow::selectedNodePath() const {
    const NodeId selected = listView_.selectedNode();
    if (selected == kInvalidNodeId || selected >= tree_.nodes().size()) {
        return {};
    }
    return tree_.node(selected).path;
}

bool MainWindow::selectNodeByPath(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    for (const FileNode& node : tree_.nodes()) {
        if (node.path == path) {
            return listView_.selectNode(node.id);
        }
    }
    return false;
}

void MainWindow::refreshChromeState() {
    std::wstring title = fileNameFromPath(history_.currentPath());
    if (title.empty()) {
        title = history_.currentPath();
    }

    chromeState_.title = std::move(title);
    chromeState_.pathText = history_.currentPath();
    chromeState_.canGoBack = history_.canGoBack();
    chromeState_.canGoForward = history_.canGoForward();

    sidebar_.refresh(homePath_, history_.currentPath());
    chromeState_.sidebarItems = sidebar_.items();
}

void MainWindow::goBack() {
    if (!history_.canGoBack()) {
        return;
    }

    const std::wstring target = history_.backTarget();
    if (!navigateToDirectory(target, HistoryMode::BackForward)) {
        return;
    }

    history_.goBack();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::goForward() {
    if (!history_.canGoForward()) {
        return;
    }

    const std::wstring target = history_.forwardTarget();
    if (!navigateToDirectory(target, HistoryMode::BackForward)) {
        return;
    }

    history_.goForward();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setStatusText(std::wstring text) {
    chromeState_.statusText = std::move(text);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::loadChildrenIfNeeded(NodeId folder) {
    if (folder == kInvalidNodeId || folder >= tree_.nodes().size()) {
        return;
    }

    const FileNode& node = tree_.node(folder);
    if (node.kind != FileKind::Folder || node.childrenLoaded) {
        return;
    }

    const DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(node.path);
    if (!result.ok()) {
        tree_.setExpanded(folder, false);
        return;
    }

    tree_.replaceChildren(folder, result.children);
}

void MainWindow::paint() {
    PAINTSTRUCT paint{};
    BeginPaint(hwnd_, &paint);

    if (!render_.isReady() && !render_.initialize(hwnd_)) {
        EndPaint(hwnd_, &paint);
        return;
    }

    const LayoutRects rects = currentLayout();

    render_.beginDraw();
    render_.clear(D2D1::ColorF(1.0f, 1.0f, 1.0f));
    chrome_.draw(render_, rects, chromeState_);
    listView_.draw(render_, rects.list);
    const bool targetLost = render_.endDraw();

    EndPaint(hwnd_, &paint);

    if (targetLost) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

}
