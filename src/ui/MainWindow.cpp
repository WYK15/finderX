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
constexpr UINT_PTR kDirectoryWatcherTimerId = 2001;
constexpr UINT_PTR kDirectoryRefreshTimerId = 2002;
constexpr UINT kDirectoryWatcherPollMs = 500;
constexpr DWORD kDirectoryChangeFilter = FILE_NOTIFY_CHANGE_FILE_NAME
    | FILE_NOTIFY_CHANGE_DIR_NAME
    | FILE_NOTIFY_CHANGE_LAST_WRITE
    | FILE_NOTIFY_CHANGE_SIZE;

bool containsPoint(const D2D1_RECT_F& rect, D2D1_POINT_2F point) {
    return point.x >= rect.left && point.x <= rect.right
        && point.y >= rect.top && point.y <= rect.bottom;
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
        const POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        const D2D1_POINT_2F dipPoint = render_.clientPointToDips(clientPoint);
        const float x = dipPoint.x;
        const float y = dipPoint.y;

        const ChromeHitResult chromeHit = chrome_.hitTest(x, y, rects, chromeState_);
        switch (chromeHit.kind) {
        case ChromeHitKind::Back:
            goBack();
            return 0;
        case ChromeHitKind::Forward:
            goForward();
            return 0;
        case ChromeHitKind::SearchField:
            focusSearch();
            return 0;
        case ChromeHitKind::SidebarItem:
            if (chromeHit.sidebarIndex < chromeState_.sidebarItems.size()) {
                navigateToDirectory(chromeState_.sidebarItems[chromeHit.sidebarIndex].path, HistoryMode::Push);
            }
            return 0;
        case ChromeHitKind::None:
            break;
        }

        const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        blurSearch();
        const ListInteractionResult result = listView_.onMouseDown(x, y, rects.list, controlDown, shiftDown);
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
        const D2D1_POINT_2F dipPoint = render_.clientPointToDips(clientPoint);
        POINT screenPoint = clientPoint;
        ClientToScreen(hwnd_, &screenPoint);
        showContextMenu(dipPoint, screenPoint);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        const LayoutRects rects = currentLayout();
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(hwnd_, &point);
        const D2D1_POINT_2F dipPoint = render_.clientPointToDips(point);
        const float x = dipPoint.x;
        const float y = dipPoint.y;
        if (x >= rects.list.left && x <= rects.list.right && y >= rects.list.top && y <= rects.list.bottom
            && listView_.onWheel(GET_WHEEL_DELTA_WPARAM(wParam))) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_COMMAND:
        handleCommand(wParam);
        return 0;
    case WM_TIMER:
        if (wParam == kDirectoryWatcherTimerId) {
            pollDirectoryWatcher();
            return 0;
        }
        if (wParam == kDirectoryRefreshTimerId) {
            consumeDirectoryRefresh();
            return 0;
        }
        return 0;
    case WM_CHAR:
        if (handleSearchChar(wParam)) {
            return 0;
        }
        return DefWindowProcW(hwnd_, message, wParam, lParam);
    case WM_KEYDOWN: {
        contextNode_ = kInvalidNodeId;
        const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (wParam == L'F' && controlDown) {
            focusSearch();
            return 0;
        }

        if (handleSearchKeyDown(wParam)) {
            return 0;
        }

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

        const ListInteractionResult result = listView_.onKeyDown(wParam, controlDown, shiftDown);
        loadChildrenIfNeeded(result.expandedFolder);
        activateNode(result.activatedNode);
        if (result.changed) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_DESTROY:
        stopDirectoryWatcher();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

LayoutRects MainWindow::currentLayout() const {
    const D2D1_SIZE_F renderSize = render_.sizeDips();
    if (renderSize.width > 0.0f && renderSize.height > 0.0f) {
        return chrome_.layout(renderSize.width, renderSize.height);
    }

    RECT client{};
    if (!hwnd_ || !GetClientRect(hwnd_, &client)) {
        return chrome_.layout(0.0f, 0.0f);
    }

    const D2D1_SIZE_F clientSize = pixelsToDips(
        static_cast<unsigned int>(client.right - client.left),
        static_cast<unsigned int>(client.bottom - client.top),
        render_.dpiScale());
    return chrome_.layout(clientSize.width, clientSize.height);
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

    const std::wstring watchedPath = path;
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
    searchText_.clear();
    searchFocused_ = false;
    listView_.setFilterText(L"");

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
    startDirectoryWatcher(watchedPath);
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

void MainWindow::showContextMenu(D2D1_POINT_2F clientPoint, POINT screenPoint) {
    const LayoutRects rects = currentLayout();
    if (!containsPoint(rects.list, clientPoint)) {
        return;
    }

    contextNode_ = listView_.nodeAtPoint(
        clientPoint.x,
        clientPoint.y,
        rects.list);

    const bool hasTarget = contextNode_ != kInvalidNodeId;
    if (hasTarget && !listView_.isSelected(contextNode_) && listView_.selectNode(contextNode_)) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    const std::vector<NodeId> targets = commandTargetNodes(true);
    const bool singleTarget = targets.size() == 1;
    if (hasTarget) {
        AppendMenuW(menu, MF_STRING, kCommandOpen, L"Open");
        if (singleTarget) {
            AppendMenuW(menu, MF_STRING, kCommandRename, L"Rename");
        }
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

std::vector<NodeId> MainWindow::commandTargetNodes(bool includeSelection) const {
    if (contextNode_ != kInvalidNodeId && contextNode_ < tree_.nodes().size()) {
        if (includeSelection && listView_.isSelected(contextNode_)) {
            return listView_.selectedNodes();
        }
        return {contextNode_};
    }

    if (includeSelection) {
        return listView_.selectedNodes();
    }

    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return {};
    }
    return {target};
}

std::vector<std::wstring> MainWindow::pathsForNodes(std::span<const NodeId> nodes) const {
    std::vector<std::wstring> paths;
    paths.reserve(nodes.size());
    for (const NodeId node : nodes) {
        if (node != kInvalidNodeId && node < tree_.nodes().size()) {
            paths.push_back(tree_.node(node).path);
        }
    }
    return paths;
}

void MainWindow::openContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }
    activateNode(target);
}

void MainWindow::renameContextNode() {
    const std::vector<NodeId> targets = commandTargetNodes(true);
    if (targets.size() != 1 || targets.front() >= tree_.nodes().size()) {
        return;
    }

    const NodeId target = targets.front();
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
    const std::vector<NodeId> targets = commandTargetNodes(true);
    const std::vector<std::wstring> paths = pathsForNodes(targets);
    if (paths.empty()) {
        return;
    }

    const shell::FileOperationResult result = shell::moveToTrash(hwnd_, paths);
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot move items to trash" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting(std::span<const std::wstring>{});
}

void MainWindow::copyContextNode() {
    const std::vector<NodeId> targets = commandTargetNodes(true);
    std::vector<std::wstring> paths = pathsForNodes(targets);
    if (paths.empty()) {
        return;
    }

    fileOperationState_.setCopy(std::move(paths));
}

void MainWindow::cutContextNode() {
    const std::vector<NodeId> targets = commandTargetNodes(true);
    std::vector<std::wstring> paths = pathsForNodes(targets);
    if (paths.empty()) {
        return;
    }

    fileOperationState_.setCut(std::move(paths));
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

    const std::vector<std::wstring>& sourcePaths = fileOperationState_.sourcePaths();
    if (sourcePaths.empty()) {
        setStatusText(L"Cannot paste here");
        return;
    }

    shell::FileOperationResult result;
    if (fileOperationState_.kind() == ui::PendingFileOperationKind::Copy) {
        result = shell::copyToDirectory(hwnd_, sourcePaths, currentPath);
    } else if (fileOperationState_.kind() == ui::PendingFileOperationKind::Cut) {
        result = shell::moveToDirectory(hwnd_, sourcePaths, currentPath);
    } else {
        setStatusText(L"Cannot paste here");
        return;
    }

    if (!result.success) {
        const wchar_t* fallback = fileOperationState_.kind() == ui::PendingFileOperationKind::Copy
            ? L"Cannot copy items"
            : L"Cannot move items";
        setStatusText(result.message.empty() ? fallback : result.message);
        return;
    }

    fileOperationState_.markPasteSucceeded();
    refreshCurrentDirectory();
}

void MainWindow::revealContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    if (!shell::revealInExplorer(hwnd_, tree_.node(target).path)) {
        setStatusText(L"Cannot show in Explorer");
    }
}

void MainWindow::copyContextNodePath() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    if (!shell::copyPathToClipboard(hwnd_, tree_.node(target).path)) {
        setStatusText(L"Cannot copy path");
    }
}

bool MainWindow::refreshCurrentDirectory() {
    const std::vector<NodeId> selectedNodes = listView_.selectedNodes();
    const std::vector<std::wstring> selectedPaths = pathsForNodes(selectedNodes);
    return refreshCurrentDirectorySelecting(selectedPaths);
}

bool MainWindow::refreshCurrentDirectorySelecting(const std::wstring& selectedPath) {
    if (selectedPath.empty()) {
        return refreshCurrentDirectorySelecting(std::span<const std::wstring>{});
    }
    return refreshCurrentDirectorySelecting(std::span<const std::wstring>(&selectedPath, 1));
}

bool MainWindow::refreshCurrentDirectorySelecting(std::span<const std::wstring> selectedPaths) {
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
    listView_.setFilterText(searchText_);
    std::vector<NodeId> restored;
    restored.reserve(selectedPaths.size());
    for (const std::wstring& selectedPath : selectedPaths) {
        if (selectedPath.empty()) {
            continue;
        }
        for (const FileNode& node : tree_.nodes()) {
            if (node.path == selectedPath) {
                restored.push_back(node.id);
                break;
            }
        }
    }
    listView_.selectNodes(restored);
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
    chromeState_.searchText = searchText_;
    chromeState_.searchFocused = searchFocused_;
}

void MainWindow::focusSearch() {
    searchFocused_ = true;
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::blurSearch() {
    if (!searchFocused_) {
        return;
    }

    searchFocused_ = false;
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::clearSearch() {
    setSearchText(L"");
}

void MainWindow::setSearchText(std::wstring text) {
    if (searchText_ == text) {
        return;
    }

    searchText_ = std::move(text);
    listView_.setFilterText(searchText_);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool MainWindow::handleSearchKeyDown(WPARAM key) {
    if (!searchFocused_) {
        return false;
    }

    switch (key) {
    case VK_BACK:
        if (!searchText_.empty()) {
            std::wstring next = searchText_;
            next.pop_back();
            setSearchText(std::move(next));
        }
        return true;
    case VK_ESCAPE:
        if (!searchText_.empty()) {
            clearSearch();
        } else {
            blurSearch();
        }
        return true;
    case VK_RETURN:
        blurSearch();
        return true;
    default:
        return true;
    }
}

bool MainWindow::handleSearchChar(WPARAM character) {
    if (!searchFocused_) {
        return false;
    }

    if (character >= 32 && character != 127) {
        std::wstring next = searchText_;
        next.push_back(static_cast<wchar_t>(character));
        setSearchText(std::move(next));
    }
    return true;
}

void MainWindow::startDirectoryWatcher(const std::wstring& path) {
    stopDirectoryWatcher();
    if (!hwnd_ || path.empty()) {
        return;
    }

    directoryChangeHandle_ = FindFirstChangeNotificationW(
        path.c_str(),
        FALSE,
        kDirectoryChangeFilter);
    if (directoryChangeHandle_ == INVALID_HANDLE_VALUE) {
        return;
    }

    SetTimer(hwnd_, kDirectoryWatcherTimerId, kDirectoryWatcherPollMs, nullptr);
}

void MainWindow::stopDirectoryWatcher() {
    if (hwnd_) {
        KillTimer(hwnd_, kDirectoryWatcherTimerId);
        KillTimer(hwnd_, kDirectoryRefreshTimerId);
    }

    directoryRefreshDebouncer_.cancel();

    if (directoryChangeHandle_ != INVALID_HANDLE_VALUE) {
        FindCloseChangeNotification(directoryChangeHandle_);
        directoryChangeHandle_ = INVALID_HANDLE_VALUE;
    }
}

void MainWindow::pollDirectoryWatcher() {
    if (directoryChangeHandle_ == INVALID_HANDLE_VALUE) {
        return;
    }

    const DWORD waitResult = WaitForSingleObject(directoryChangeHandle_, 0);
    if (waitResult != WAIT_OBJECT_0) {
        return;
    }

    scheduleDirectoryRefresh();
    if (!FindNextChangeNotification(directoryChangeHandle_)) {
        stopDirectoryWatcher();
    }
}

void MainWindow::scheduleDirectoryRefresh() {
    directoryRefreshDebouncer_.request(GetTickCount64());
    SetTimer(
        hwnd_,
        kDirectoryRefreshTimerId,
        static_cast<UINT>(ui::kDefaultDirectoryRefreshDebounceMs),
        nullptr);
}

void MainWindow::consumeDirectoryRefresh() {
    if (!directoryRefreshDebouncer_.consumeIfDue(GetTickCount64())) {
        return;
    }

    KillTimer(hwnd_, kDirectoryRefreshTimerId);
    refreshCurrentDirectory();
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
