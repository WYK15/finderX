#pragma once

#include "fs/DirectoryLoader.h"
#include "model/FileTree.h"
#include "navigation/NavigationHistory.h"
#include "navigation/SidebarModel.h"
#include "ui/FileOperationState.h"
#include "ui/FinderChrome.h"
#include "ui/FinderListView.h"
#include "ui/RenderContext.h"

#include <span>
#include <string>
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

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LayoutRects currentLayout() const;
    void initializeFileTree();
    bool navigateToDirectory(std::wstring path, HistoryMode mode);
    void activateNode(NodeId nodeId);
    void openFile(const std::wstring& path);
    void showContextMenu(POINT clientPoint, POINT screenPoint);
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
    void goBack();
    void goForward();
    void setStatusText(std::wstring text);
    void loadChildrenIfNeeded(NodeId folder);
    void paint();

    HWND hwnd_ = nullptr;
    RenderContext render_;
    FinderChrome chrome_;
    DirectoryLoader directoryLoader_;
    FileTree tree_;
    FinderListView listView_{&tree_};
    NavigationHistory history_;
    SidebarModel sidebar_;
    ChromeState chromeState_;
    ui::FileOperationState fileOperationState_;
    std::wstring homePath_;
    std::wstring searchText_;
    bool searchFocused_ = false;
    NodeId contextNode_ = kInvalidNodeId;
};

}
