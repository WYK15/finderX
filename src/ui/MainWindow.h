#pragma once

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
    void paint();

    HWND hwnd_ = nullptr;
    RenderContext render_;
    FinderChrome chrome_;
    FileTree tree_ = FileTree::sample();
    FinderListView listView_{&tree_};
};

}
