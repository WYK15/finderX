#pragma once

#include "fs/DirectoryLoader.h"
#include "settings/AppSettings.h"
#include "model/FileTree.h"
#include "navigation/NavigationHistory.h"
#include "navigation/SidebarModel.h"
#include "ui/AddressEditor.h"
#include "ui/FileOperationState.h"
#include "ui/DirectoryRefreshDebouncer.h"
#include "ui/FinderChrome.h"
#include "ui/FinderListView.h"
#include "ui/RenderContext.h"
#include "ui/SettingsDialog.h"

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

    enum class ColumnResizeTarget {
        None,
        Modified,
        Size,
        Kind
    };

    enum class PointerMode {
        None,
        PendingMove,
        MovingItems,
        RubberBand,
        AddressSelecting
    };

    struct TabState {
        enum class LocationKind {
            Directory,
            ThisPc
        };

        FileTree tree;
        FinderListView listView;
        NavigationHistory history;
        std::wstring searchText;
        bool searchFocused = false;
        bool searchCaretVisible = false;
        ui::AddressEditor addressEditor;
        std::wstring statusText;
        LocationKind locationKind = LocationKind::Directory;

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
    bool createTabAtPath(const std::wstring& path);
    void activateTab(std::size_t index);
    void closeTab(std::size_t index);
    void ensureActiveTabAfterClose(std::size_t closedIndex, std::size_t oldActiveIndex);
    bool navigateToThisPc(HistoryMode mode);
    bool navigateToLocation(std::wstring path, HistoryMode mode);
    bool navigateToDirectory(std::wstring path, HistoryMode mode);
    bool isActiveDirectoryLocation() const;
    void activateNode(NodeId nodeId);
    void openFile(const std::wstring& path);
    void showContextMenu(D2D1_POINT_2F clientPoint, POINT screenPoint);
    void showSidebarContextMenu(std::size_t sidebarIndex, POINT screenPoint);
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
    void compressContextNodesToZip();
    void extractContextZipHere();
    void openContextNodeInNewTab();
    void createShortcutForContextNode();
    void createFolderInCurrentDirectory();
    void createFileInCurrentDirectory();
    void addPathToFavorites(const std::wstring& path);
    std::wstring favoriteLabelForPath(const std::wstring& path) const;
    void addCurrentDirectoryToFavorites();
    void removeContextFavorite();
    void openPowerShellForContext();
    std::wstring powerShellTargetDirectory() const;
    void revealContextNode();
    void copyContextNodePath();
    void finishPointerInteraction(D2D1_POINT_2F point);
    void moveDraggedItemsToPath(const std::wstring& destinationPath);
    bool canMoveDraggedItemsTo(NodeId destinationNode) const;
    bool canMoveDraggedItemsToPath(const std::wstring& destinationPath) const;
    void updateDragTarget(NodeId destinationNode);
    void updateDragTargetPath(std::wstring destinationPath, std::wstring destinationName);
    void updateDragTargetAtPoint(D2D1_POINT_2F point, const LayoutRects& rects);
    D2D1_RECT_F currentRubberBandRect() const;
    std::wstring currentDragFeedbackText() const;
    void drawDragFeedback();
    void applySort(std::vector<FileNode>& nodes) const;
    ListViewStyle currentListViewStyle() const;
    void applyListStyle(TabState& tab) const;
    void applyVisualSettings();
    void openSettingsDialog();
    void changeSort(SortColumn column);
    void setSortDirection(SortDirection direction);
    void showSortMenu(POINT screenPoint);
    bool saveSettingsOrStatus();
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
    void focusAddress();
    void focusAddressAtPoint(float x);
    void blurAddress();
    void commitAddress();
    bool handleAddressCharacter(wchar_t character);
    bool handleAddressKeyDown(WPARAM key, bool controlDown);
    void updateAddressSelectionAtPoint(float x);
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
    void restoreExpandedFolders(TabState& tab, std::vector<std::wstring> expandedPaths);
    void paint();

    HWND hwnd_ = nullptr;
    RenderContext render_;
    FinderChrome chrome_;
    DirectoryLoader directoryLoader_;
    SidebarModel sidebar_;
    ChromeState chromeState_;
    AppSettings settings_;
    ui::FileOperationState fileOperationState_;
    std::wstring homePath_;
    std::vector<std::unique_ptr<TabState>> tabs_;
    std::size_t activeTabIndex_ = 0;
    ui::DirectoryRefreshDebouncer directoryRefreshDebouncer_{ui::kDefaultDirectoryRefreshDebounceMs};
    HANDLE directoryChangeHandle_ = INVALID_HANDLE_VALUE;
    NodeId contextNode_ = kInvalidNodeId;
    std::wstring contextNodePath_;
    std::wstring contextFavoritePath_;
    ColumnResizeTarget columnResizeTarget_ = ColumnResizeTarget::None;
    float columnResizeStartX_ = 0.0f;
    float columnResizeStartWidth_ = 0.0f;
    PointerMode pointerMode_ = PointerMode::None;
    D2D1_POINT_2F pointerStart_{};
    D2D1_POINT_2F pointerCurrent_{};
    bool rubberBandAdditive_ = false;
    std::vector<NodeId> draggedNodes_;
    NodeId dragTargetNode_ = kInvalidNodeId;
    std::wstring dragTargetPath_;
    std::wstring dragFeedbackText_;
};

}
