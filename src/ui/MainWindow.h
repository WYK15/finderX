#pragma once

#include "fs/DirectoryLoader.h"
#include "model/FileTree.h"
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
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LayoutRects currentLayout() const;
    void initializeFileTree();
    void loadChildrenIfNeeded(NodeId folder);
    void paint();

    HWND hwnd_ = nullptr;
    RenderContext render_;
    FinderChrome chrome_;
    DirectoryLoader directoryLoader_;
    FileTree tree_;
    FinderListView listView_{&tree_};
};

}
