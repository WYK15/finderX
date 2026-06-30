#include "ui/MainWindow.h"

#include <utility>

#include <shellapi.h>
#include <windowsx.h>

namespace finderx {

namespace {

constexpr wchar_t kWindowClassName[] = L"FinderXMainWindow";

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
    case WM_KEYDOWN: {
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

bool MainWindow::navigateToDirectory(const std::wstring& path, HistoryMode mode) {
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
        history_.setInitialPath(path);
        break;
    case HistoryMode::Push:
        history_.navigateTo(path);
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
    const HINSTANCE result = ShellExecuteW(hwnd_, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        setStatusText(L"Cannot open file");
    }
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
