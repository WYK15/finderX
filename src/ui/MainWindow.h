#pragma once

#include "fs/DirectoryLoader.h"
#include "model/FileTree.h"
#include "navigation/NavigationHistory.h"
#include "navigation/SidebarModel.h"
#include "ui/FinderChrome.h"
#include "ui/FinderListView.h"
#include "ui/RenderContext.h"

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
    void openContextNode();
    void revealContextNode();
    void copyContextNodePath();
    bool refreshCurrentDirectory();
    std::wstring selectedNodePath() const;
    bool selectNodeByPath(const std::wstring& path);
    void refreshChromeState();
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
    std::wstring homePath_;
    NodeId contextNode_ = kInvalidNodeId;
};

}
