#pragma once

#include "fs/DirectoryLoader.h"
#include "model/FileTree.h"
#include "navigation/NavigationHistory.h"
#include "navigation/SidebarModel.h"
#include "ui/FileOperationState.h"
#include "ui/DirectoryRefreshDebouncer.h"
#include "ui/FinderChrome.h"
#include "ui/FinderListView.h"
#include "ui/RenderContext.h"

#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

namespace finderx {

class MainWindow {
public:
    bool create(HINSTANCE instance, int showCommand);
    HWND hwnd() const;

private:
    enum class HistoryMode {
        Initial,
        Push,
        Replace,
        BackForward
    };

    struct TabState {
        FileTree tree;
        FinderListView listView;
        NavigationHistory history;
        std::wstring searchText;
        bool searchFocused = false;
        std::wstring statusText;

        TabState(std::wstring rootPath, std::wstring rootName)
            : tree(std::move(rootPath), std::move(rootName)),
              listView(&tree) {}
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LayoutRects currentLayout() const;
    TabState& activeTab();
    const TabState& activeTab() const;
    bool hasActiveTab() const;
    std::vector<std::wstring> tabTitles() const;
    void initializeFileTree();
    bool navigateToDirectory(std::wstring path, HistoryMode mode);
    void activateNode(NodeId nodeId);
    void openFile(const std::wstring& path);
    void showContextMenu(D2D1_POINT_2F clientPoint, POINT screenPoint);
    void handleCommand(WPARAM wParam);
    NodeId commandTargetNode() const;
    std::vector<NodeId> commandTargetNodes(bool includeSelection) const;
    std::vector<std::wstring> pathsForNodes(std::span<const NodeId> nodes) const;
    void openContextNode();
    void renameContextNode();
    void moveContextNodeToTrash();
    void copyContextNode();
    void cutContextNode();
    void pasteIntoCurrentDirectory();
    void revealContextNode();
    void copyContextNodePath();
    bool refreshCurrentDirectory();
    bool refreshCurrentDirectorySelecting(const std::wstring& selectedPath);
    bool refreshCurrentDirectorySelecting(std::span<const std::wstring> selectedPaths);
    std::wstring selectedNodePath() const;
    bool selectNodeByPath(const std::wstring& path);
    void refreshChromeState();
    void focusSearch();
    void blurSearch();
    void clearSearch();
    void setSearchText(std::wstring text);
    bool handleSearchKeyDown(WPARAM key);
    bool handleSearchChar(WPARAM character);
    void startDirectoryWatcher(const std::wstring& path);
    void stopDirectoryWatcher();
    void pollDirectoryWatcher();
    void scheduleDirectoryRefresh();
    void consumeDirectoryRefresh();
    void goBack();
    void goForward();
    void setStatusText(std::wstring text);
    void loadChildrenIfNeeded(NodeId folder);
    void paint();

    HWND hwnd_ = nullptr;
    RenderContext render_;
    FinderChrome chrome_;
    DirectoryLoader directoryLoader_;
    SidebarModel sidebar_;
    ChromeState chromeState_;
    ui::FileOperationState fileOperationState_;
    std::wstring homePath_;
    std::vector<std::unique_ptr<TabState>> tabs_;
    std::size_t activeTabIndex_ = 0;
    ui::DirectoryRefreshDebouncer directoryRefreshDebouncer_{ui::kDefaultDirectoryRefreshDebounceMs};
    HANDLE directoryChangeHandle_ = INVALID_HANDLE_VALUE;
    NodeId contextNode_ = kInvalidNodeId;
};

}
