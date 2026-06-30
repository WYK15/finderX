#include "ui/MainWindow.h"

#include "fs/DriveEnumerator.h"
#include "fs/FileCreation.h"
#include "fs/FileSorter.h"
#include "settings/AppSettings.h"
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
constexpr UINT kCommandNewFolder = 1010;
constexpr UINT kCommandNewFile = 1011;
constexpr UINT kCommandOpenPowerShell = 1012;
constexpr UINT kCommandSortName = 1013;
constexpr UINT kCommandSortModified = 1014;
constexpr UINT kCommandSortSize = 1015;
constexpr UINT kCommandSortKind = 1016;
constexpr UINT kCommandSortAscending = 1017;
constexpr UINT kCommandSortDescending = 1018;
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

MainWindow::TabState& MainWindow::activeTab() {
    return *tabs_.at(activeTabIndex_);
}

const MainWindow::TabState& MainWindow::activeTab() const {
    return *tabs_.at(activeTabIndex_);
}

bool MainWindow::hasActiveTab() const {
    return activeTabIndex_ < tabs_.size() && tabs_[activeTabIndex_] != nullptr;
}

std::vector<std::wstring> MainWindow::tabTitles() const {
    std::vector<std::wstring> titles;
    titles.reserve(tabs_.size());
    for (const std::unique_ptr<TabState>& tab : tabs_) {
        if (!tab) {
            titles.push_back({});
            continue;
        }

        const std::wstring& path = tab->history.currentPath();
        std::wstring title = path == thisPcPath() ? L"This PC" : fileNameFromPath(path);
        if (title.empty()) {
            title = path;
        }
        titles.push_back(std::move(title));
    }
    return titles;
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
        POINT screenPoint = clientPoint;
        ClientToScreen(hwnd_, &screenPoint);

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
                navigateToLocation(chromeState_.sidebarItems[chromeHit.sidebarIndex].path, HistoryMode::Push);
            }
            return 0;
        case ChromeHitKind::Tab:
            activateTab(chromeHit.tabIndex);
            return 0;
        case ChromeHitKind::NewTab:
            createTabAtPath(hasActiveTab() && activeTab().locationKind != TabState::LocationKind::ThisPc
                ? activeTab().history.currentPath()
                : homePath_);
            return 0;
        case ChromeHitKind::SortMenu:
            showSortMenu(screenPoint);
            return 0;
        case ChromeHitKind::Settings:
            return 0;
        case ChromeHitKind::HeaderName:
            changeSort(SortColumn::Name);
            return 0;
        case ChromeHitKind::HeaderModified:
            changeSort(SortColumn::Modified);
            return 0;
        case ChromeHitKind::HeaderSize:
            changeSort(SortColumn::Size);
            return 0;
        case ChromeHitKind::HeaderKind:
            changeSort(SortColumn::Kind);
            return 0;
        case ChromeHitKind::None:
            break;
        }

        const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (!hasActiveTab()) {
            return 0;
        }
        blurSearch();
        const ListInteractionResult result = activeTab().listView.onMouseDown(x, y, rects.list, controlDown, shiftDown);
        loadChildrenIfNeeded(result.expandedFolder);
        activateNode(result.activatedNode);
        if (result.changed) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_RBUTTONDOWN: {
        SetFocus(hwnd_);
        if (!hasActiveTab()) {
            return 0;
        }
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
            && hasActiveTab()
            && activeTab().listView.onWheel(GET_WHEEL_DELTA_WPARAM(wParam))) {
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
        if (!hasActiveTab()) {
            return DefWindowProcW(hwnd_, message, wParam, lParam);
        }
        if (handleSearchChar(wParam)) {
            return 0;
        }
        return DefWindowProcW(hwnd_, message, wParam, lParam);
    case WM_KEYDOWN: {
        contextNode_ = kInvalidNodeId;
        const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (controlDown && wParam == L'T') {
            createTabAtPath(hasActiveTab() && activeTab().locationKind != TabState::LocationKind::ThisPc
                ? activeTab().history.currentPath()
                : homePath_);
            return 0;
        }

        if (!hasActiveTab()) {
            return DefWindowProcW(hwnd_, message, wParam, lParam);
        }

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

        if (wParam == VK_F2 && isActiveDirectoryLocation()) {
            renameContextNode();
            return 0;
        }

        if (wParam == VK_DELETE && isActiveDirectoryLocation()) {
            moveContextNodeToTrash();
            return 0;
        }

        if (wParam == L'C' && controlDown && isActiveDirectoryLocation()) {
            copyContextNode();
            return 0;
        }

        if (wParam == L'X' && controlDown && isActiveDirectoryLocation()) {
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

        const ListInteractionResult result = activeTab().listView.onKeyDown(wParam, controlDown, shiftDown);
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
    settings_ = loadSettings(homePath_).settings;
    createTabAtPath(homePath_);
}

bool MainWindow::createTabAtPath(const std::wstring& path) {
    const std::wstring target = path.empty() ? homePath_ : path;
    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(target);
    if (!result.ok()) {
        setStatusText(L"Cannot open folder");
        return false;
    }
    applySort(result.children);

    std::wstring rootName = fileNameFromPath(target);
    if (rootName.empty()) {
        rootName = target;
    }

    auto tab = std::make_unique<TabState>(target, std::move(rootName));
    tab->tree.replaceChildren(tab->tree.rootId(), std::move(result.children));
    tab->history.setInitialPath(target);

    tabs_.push_back(std::move(tab));
    activeTabIndex_ = tabs_.size() - 1;
    startDirectoryWatcher(target);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

void MainWindow::activateTab(std::size_t index) {
    if (index >= tabs_.size() || index == activeTabIndex_) {
        return;
    }

    if (hasActiveTab()) {
        activeTab().searchFocused = false;
    }

    activeTabIndex_ = index;
    if (activeTab().locationKind == TabState::LocationKind::Directory) {
        startDirectoryWatcher(activeTab().history.currentPath());
    } else {
        stopDirectoryWatcher();
    }
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool MainWindow::navigateToThisPc(HistoryMode mode) {
    const std::wstring path = thisPcPath();
    if (!hasActiveTab()) {
        tabs_.push_back(std::make_unique<TabState>(path, L"This PC"));
        activeTabIndex_ = tabs_.size() - 1;
    }

    TabState& tab = activeTab();
    tab.locationKind = TabState::LocationKind::ThisPc;
    tab.tree = FileTree(path, L"This PC");
    tab.tree.replaceChildren(tab.tree.rootId(), enumerateDriveNodes());
    tab.listView = FinderListView(&tab.tree);
    tab.searchText.clear();
    tab.searchFocused = false;
    tab.listView.setFilterText(L"");

    switch (mode) {
    case HistoryMode::Initial:
    case HistoryMode::Replace:
        tab.history.setInitialPath(path);
        break;
    case HistoryMode::Push:
        tab.history.navigateTo(path);
        break;
    case HistoryMode::BackForward:
        break;
    }

    tab.statusText.clear();
    contextNode_ = kInvalidNodeId;
    stopDirectoryWatcher();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

bool MainWindow::navigateToLocation(std::wstring path, HistoryMode mode) {
    if (path == thisPcPath()) {
        return navigateToThisPc(mode);
    }
    return navigateToDirectory(std::move(path), mode);
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
    applySort(result.children);

    std::wstring rootName = fileNameFromPath(path);
    if (rootName.empty()) {
        rootName = path;
    }

    if (!hasActiveTab()) {
        tabs_.push_back(std::make_unique<TabState>(path, rootName));
        activeTabIndex_ = tabs_.size() - 1;
    }

    TabState& tab = activeTab();
    tab.locationKind = TabState::LocationKind::Directory;
    tab.tree = FileTree(path, std::move(rootName));
    tab.tree.replaceChildren(tab.tree.rootId(), std::move(result.children));
    tab.listView = FinderListView(&tab.tree);
    tab.searchText.clear();
    tab.searchFocused = false;
    tab.listView.setFilterText(L"");

    switch (mode) {
    case HistoryMode::Initial:
    case HistoryMode::Replace:
        tab.history.setInitialPath(std::move(path));
        break;
    case HistoryMode::Push:
        tab.history.navigateTo(std::move(path));
        break;
    case HistoryMode::BackForward:
        break;
    }

    tab.statusText.clear();
    startDirectoryWatcher(watchedPath);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

bool MainWindow::isActiveDirectoryLocation() const {
    return hasActiveTab() && activeTab().locationKind == TabState::LocationKind::Directory;
}

void MainWindow::activateNode(NodeId nodeId) {
    TabState& tab = activeTab();
    if (nodeId == kInvalidNodeId || nodeId >= tab.tree.nodes().size()) {
        return;
    }

    const FileNode& node = tab.tree.node(nodeId);
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

    TabState& tab = activeTab();
    contextNode_ = tab.listView.nodeAtPoint(
        clientPoint.x,
        clientPoint.y,
        rects.list);

    const bool hasTarget = contextNode_ != kInvalidNodeId;
    if (hasTarget && !tab.listView.isSelected(contextNode_) && tab.listView.selectNode(contextNode_)) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    const std::vector<NodeId> targets = commandTargetNodes(true);
    const bool singleTarget = targets.size() == 1;
    const bool directoryLocation = isActiveDirectoryLocation();
    if (hasTarget) {
        AppendMenuW(menu, MF_STRING, kCommandOpen, L"Open");
        if (directoryLocation) {
            if (singleTarget) {
                AppendMenuW(menu, MF_STRING, kCommandRename, L"Rename");
            }
            AppendMenuW(menu, MF_STRING, kCommandCopy, L"Copy");
            AppendMenuW(menu, MF_STRING, kCommandCut, L"Cut");
            if (fileOperationState_.hasPendingOperation()) {
                AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
            }
            if (singleTarget && targets.front() < tab.tree.nodes().size()
                && tab.tree.node(targets.front()).kind == FileKind::Folder) {
                AppendMenuW(menu, MF_STRING, kCommandOpenPowerShell, L"Open in PowerShell");
            }
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kCommandReveal, L"Show in Explorer");
        AppendMenuW(menu, MF_STRING, kCommandCopyPath, L"Copy Path");
        if (directoryLocation) {
            AppendMenuW(menu, MF_STRING, kCommandMoveToTrash, L"Move to Trash");
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    } else if (directoryLocation) {
        AppendMenuW(menu, MF_STRING, kCommandNewFolder, L"New Folder");
        AppendMenuW(menu, MF_STRING, kCommandNewFile, L"New File");
        AppendMenuW(menu, MF_STRING, kCommandOpenPowerShell, L"Open in PowerShell");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        if (fileOperationState_.hasPendingOperation()) {
            AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        }
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
    case kCommandNewFolder:
        createFolderInCurrentDirectory();
        break;
    case kCommandNewFile:
        createFileInCurrentDirectory();
        break;
    case kCommandOpenPowerShell:
        openPowerShellForContext();
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
    case kCommandSortName:
        changeSort(SortColumn::Name);
        break;
    case kCommandSortModified:
        changeSort(SortColumn::Modified);
        break;
    case kCommandSortSize:
        changeSort(SortColumn::Size);
        break;
    case kCommandSortKind:
        changeSort(SortColumn::Kind);
        break;
    case kCommandSortAscending:
        setSortDirection(SortDirection::Ascending);
        break;
    case kCommandSortDescending:
        setSortDirection(SortDirection::Descending);
        break;
    default:
        break;
    }
}

NodeId MainWindow::commandTargetNode() const {
    const TabState& tab = activeTab();
    if (contextNode_ != kInvalidNodeId && contextNode_ < tab.tree.nodes().size()) {
        return contextNode_;
    }

    return tab.listView.selectedNode();
}

std::vector<NodeId> MainWindow::commandTargetNodes(bool includeSelection) const {
    const TabState& tab = activeTab();
    if (contextNode_ != kInvalidNodeId && contextNode_ < tab.tree.nodes().size()) {
        if (includeSelection && tab.listView.isSelected(contextNode_)) {
            return tab.listView.selectedNodes();
        }
        return {contextNode_};
    }

    if (includeSelection) {
        return tab.listView.selectedNodes();
    }

    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tab.tree.nodes().size()) {
        return {};
    }
    return {target};
}

std::vector<std::wstring> MainWindow::pathsForNodes(std::span<const NodeId> nodes) const {
    std::vector<std::wstring> paths;
    paths.reserve(nodes.size());
    const TabState& tab = activeTab();
    for (const NodeId node : nodes) {
        if (node != kInvalidNodeId && node < tab.tree.nodes().size()) {
            paths.push_back(tab.tree.node(node).path);
        }
    }
    return paths;
}

void MainWindow::openContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= activeTab().tree.nodes().size()) {
        return;
    }
    activateNode(target);
}

void MainWindow::renameContextNode() {
    if (!isActiveDirectoryLocation()) {
        return;
    }

    TabState& tab = activeTab();
    const std::vector<NodeId> targets = commandTargetNodes(true);
    if (targets.size() != 1 || targets.front() >= tab.tree.nodes().size()) {
        return;
    }

    const NodeId target = targets.front();
    const FileNode& node = tab.tree.node(target);
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
    if (!isActiveDirectoryLocation()) {
        return;
    }

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
    if (!isActiveDirectoryLocation()) {
        return;
    }

    const std::vector<NodeId> targets = commandTargetNodes(true);
    std::vector<std::wstring> paths = pathsForNodes(targets);
    if (paths.empty()) {
        return;
    }

    fileOperationState_.setCopy(std::move(paths));
}

void MainWindow::cutContextNode() {
    if (!isActiveDirectoryLocation()) {
        return;
    }

    const std::vector<NodeId> targets = commandTargetNodes(true);
    std::vector<std::wstring> paths = pathsForNodes(targets);
    if (paths.empty()) {
        return;
    }

    fileOperationState_.setCut(std::move(paths));
}

void MainWindow::pasteIntoCurrentDirectory() {
    if (!isActiveDirectoryLocation()) {
        setStatusText(L"Cannot paste here");
        return;
    }

    if (!fileOperationState_.hasPendingOperation()) {
        return;
    }

    const std::wstring currentPath = activeTab().history.currentPath();
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

void MainWindow::createFolderInCurrentDirectory() {
    if (!isActiveDirectoryLocation()) {
        setStatusText(L"Cannot create folder here");
        return;
    }

    const FileCreationResult result = createNewFolder(activeTab().history.currentPath());
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot create folder" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting(result.path);
}

void MainWindow::createFileInCurrentDirectory() {
    if (!isActiveDirectoryLocation()) {
        setStatusText(L"Cannot create file here");
        return;
    }

    const FileCreationResult result = createNewTextFile(activeTab().history.currentPath());
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot create file" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting(result.path);
}

std::wstring MainWindow::powerShellTargetDirectory() const {
    if (!isActiveDirectoryLocation()) {
        return {};
    }

    const TabState& tab = activeTab();
    if (contextNode_ != kInvalidNodeId && contextNode_ < tab.tree.nodes().size()) {
        const FileNode& node = tab.tree.node(contextNode_);
        if (node.kind == FileKind::Folder) {
            return node.path;
        }
    }

    return tab.history.currentPath();
}

void MainWindow::openPowerShellForContext() {
    const std::wstring directory = powerShellTargetDirectory();
    if (directory.empty() || !shell::openPowerShellAt(hwnd_, directory)) {
        setStatusText(L"Cannot open PowerShell");
    }
}

void MainWindow::revealContextNode() {
    const NodeId target = commandTargetNode();
    const TabState& tab = activeTab();
    if (target == kInvalidNodeId || target >= tab.tree.nodes().size()) {
        return;
    }

    if (!shell::revealInExplorer(hwnd_, tab.tree.node(target).path)) {
        setStatusText(L"Cannot show in Explorer");
    }
}

void MainWindow::copyContextNodePath() {
    const NodeId target = commandTargetNode();
    const TabState& tab = activeTab();
    if (target == kInvalidNodeId || target >= tab.tree.nodes().size()) {
        return;
    }

    if (!shell::copyPathToClipboard(hwnd_, tab.tree.node(target).path)) {
        setStatusText(L"Cannot copy path");
    }
}

void MainWindow::applySort(std::vector<FileNode>& nodes) const {
    sortFileNodes(nodes, settings_.sortColumn, settings_.sortDirection);
}

void MainWindow::changeSort(SortColumn column) {
    if (settings_.sortColumn == column) {
        settings_.sortDirection = settings_.sortDirection == SortDirection::Ascending
            ? SortDirection::Descending
            : SortDirection::Ascending;
    } else {
        settings_.sortColumn = column;
        settings_.sortDirection = SortDirection::Ascending;
    }

    const bool saved = saveSettingsOrStatus();
    refreshCurrentDirectory();
    if (!saved) {
        setStatusText(L"Cannot save settings");
    }
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setSortDirection(SortDirection direction) {
    if (settings_.sortDirection == direction) {
        return;
    }

    settings_.sortDirection = direction;
    const bool saved = saveSettingsOrStatus();
    refreshCurrentDirectory();
    if (!saved) {
        setStatusText(L"Cannot save settings");
    }
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::showSortMenu(POINT screenPoint) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    const auto columnFlags = [&](SortColumn column) {
        return MF_STRING | (settings_.sortColumn == column ? MF_CHECKED : MF_UNCHECKED);
    };
    const auto directionFlags = [&](SortDirection direction) {
        return MF_STRING | (settings_.sortDirection == direction ? MF_CHECKED : MF_UNCHECKED);
    };

    AppendMenuW(menu, columnFlags(SortColumn::Name), kCommandSortName, L"Name");
    AppendMenuW(menu, columnFlags(SortColumn::Modified), kCommandSortModified, L"Date Modified");
    AppendMenuW(menu, columnFlags(SortColumn::Size), kCommandSortSize, L"Size");
    AppendMenuW(menu, columnFlags(SortColumn::Kind), kCommandSortKind, L"Kind");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, directionFlags(SortDirection::Ascending), kCommandSortAscending, L"Ascending");
    AppendMenuW(menu, directionFlags(SortDirection::Descending), kCommandSortDescending, L"Descending");

    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

bool MainWindow::saveSettingsOrStatus() {
    if (saveSettings(settings_)) {
        return true;
    }

    setStatusText(L"Cannot save settings");
    return false;
}

bool MainWindow::refreshCurrentDirectory() {
    if (!hasActiveTab()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    const std::vector<NodeId> selectedNodes = activeTab().listView.selectedNodes();
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
    if (!hasActiveTab()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    if (activeTab().locationKind == TabState::LocationKind::ThisPc) {
        return navigateToThisPc(HistoryMode::BackForward);
    }

    if (!isActiveDirectoryLocation()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    TabState& tab = activeTab();
    const std::wstring currentPath = tab.history.currentPath();
    if (currentPath.empty()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(currentPath);
    if (!result.ok()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }
    applySort(result.children);

    std::wstring rootName = fileNameFromPath(currentPath);
    if (rootName.empty()) {
        rootName = currentPath;
    }

    tab.tree = FileTree(currentPath, std::move(rootName));
    tab.tree.replaceChildren(tab.tree.rootId(), std::move(result.children));
    tab.listView = FinderListView(&tab.tree);
    tab.listView.setFilterText(tab.searchText);
    std::vector<NodeId> restored;
    restored.reserve(selectedPaths.size());
    for (const std::wstring& selectedPath : selectedPaths) {
        if (selectedPath.empty()) {
            continue;
        }
        for (const FileNode& node : tab.tree.nodes()) {
            if (node.path == selectedPath) {
                restored.push_back(node.id);
                break;
            }
        }
    }
    tab.listView.selectNodes(restored);
    contextNode_ = kInvalidNodeId;
    tab.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

std::wstring MainWindow::selectedNodePath() const {
    const TabState& tab = activeTab();
    const NodeId selected = tab.listView.selectedNode();
    if (selected == kInvalidNodeId || selected >= tab.tree.nodes().size()) {
        return {};
    }
    return tab.tree.node(selected).path;
}

bool MainWindow::selectNodeByPath(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    TabState& tab = activeTab();
    for (const FileNode& node : tab.tree.nodes()) {
        if (node.path == path) {
            return tab.listView.selectNode(node.id);
        }
    }
    return false;
}

void MainWindow::refreshChromeState() {
    if (!hasActiveTab()) {
        chromeState_.title.clear();
        chromeState_.pathText.clear();
        chromeState_.canGoBack = false;
        chromeState_.canGoForward = false;
        sidebar_.refresh(homePath_, L"");
        chromeState_.sidebarItems = sidebar_.items();
        chromeState_.searchText.clear();
        chromeState_.searchFocused = false;
        chromeState_.tabTitles = tabTitles();
        chromeState_.activeTabIndex = activeTabIndex_;
        chromeState_.sortColumn = settings_.sortColumn;
        chromeState_.sortDirection = settings_.sortDirection;
        return;
    }

    const TabState& tab = activeTab();
    const bool isThisPc = tab.history.currentPath() == thisPcPath();
    std::wstring title = isThisPc ? L"This PC" : fileNameFromPath(tab.history.currentPath());
    if (title.empty()) {
        title = tab.history.currentPath();
    }

    chromeState_.title = std::move(title);
    chromeState_.pathText = isThisPc ? L"This PC" : tab.history.currentPath();
    chromeState_.canGoBack = tab.history.canGoBack();
    chromeState_.canGoForward = tab.history.canGoForward();

    sidebar_.refresh(homePath_, tab.history.currentPath());
    chromeState_.sidebarItems = sidebar_.items();
    chromeState_.searchText = tab.searchText;
    chromeState_.searchFocused = tab.searchFocused;
    chromeState_.statusText = tab.statusText;
    chromeState_.tabTitles = tabTitles();
    chromeState_.activeTabIndex = activeTabIndex_;
    chromeState_.sortColumn = settings_.sortColumn;
    chromeState_.sortDirection = settings_.sortDirection;
}

void MainWindow::focusSearch() {
    if (!hasActiveTab()) {
        return;
    }

    activeTab().searchFocused = true;
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::blurSearch() {
    if (!hasActiveTab()) {
        return;
    }

    TabState& tab = activeTab();
    if (!tab.searchFocused) {
        return;
    }

    tab.searchFocused = false;
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::clearSearch() {
    setSearchText(L"");
}

void MainWindow::setSearchText(std::wstring text) {
    if (!hasActiveTab()) {
        return;
    }

    TabState& tab = activeTab();
    if (tab.searchText == text) {
        return;
    }

    tab.searchText = std::move(text);
    tab.listView.setFilterText(tab.searchText);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

bool MainWindow::handleSearchKeyDown(WPARAM key) {
    if (!hasActiveTab()) {
        return false;
    }

    TabState& tab = activeTab();
    if (!tab.searchFocused) {
        return false;
    }

    switch (key) {
    case VK_BACK:
        if (!tab.searchText.empty()) {
            std::wstring next = tab.searchText;
            next.pop_back();
            setSearchText(std::move(next));
        }
        return true;
    case VK_ESCAPE:
        if (!tab.searchText.empty()) {
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
    if (!hasActiveTab()) {
        return false;
    }

    TabState& tab = activeTab();
    if (!tab.searchFocused) {
        return false;
    }

    if (character >= 32 && character != 127) {
        std::wstring next = tab.searchText;
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
    if (!hasActiveTab()) {
        return;
    }

    TabState& tab = activeTab();
    if (!tab.history.canGoBack()) {
        return;
    }

    const std::wstring target = tab.history.backTarget();
    if (!navigateToLocation(target, HistoryMode::BackForward)) {
        return;
    }

    activeTab().history.goBack();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::goForward() {
    if (!hasActiveTab()) {
        return;
    }

    TabState& tab = activeTab();
    if (!tab.history.canGoForward()) {
        return;
    }

    const std::wstring target = tab.history.forwardTarget();
    if (!navigateToLocation(target, HistoryMode::BackForward)) {
        return;
    }

    activeTab().history.goForward();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setStatusText(std::wstring text) {
    if (hasActiveTab()) {
        activeTab().statusText = std::move(text);
    } else {
        chromeState_.statusText = std::move(text);
    }
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::loadChildrenIfNeeded(NodeId folder) {
    TabState& tab = activeTab();
    if (folder == kInvalidNodeId || folder >= tab.tree.nodes().size()) {
        return;
    }

    const FileNode& node = tab.tree.node(folder);
    if (node.kind != FileKind::Folder || node.childrenLoaded) {
        return;
    }

    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(node.path);
    if (!result.ok()) {
        tab.tree.setExpanded(folder, false);
        return;
    }

    applySort(result.children);
    tab.tree.replaceChildren(folder, std::move(result.children));
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
    if (hasActiveTab()) {
        activeTab().listView.draw(render_, rects.list);
    }
    const bool targetLost = render_.endDraw();

    EndPaint(hwnd_, &paint);

    if (targetLost) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

}
