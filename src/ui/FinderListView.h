#pragma once

#include "model/FileTree.h"
#include "ui/RenderContext.h"

#include <d2d1.h>

namespace finderx {

class FinderListView {
public:
    explicit FinderListView(FileTree* tree);

    void draw(RenderContext& render, const D2D1_RECT_F& bounds);

private:
    FileTree* tree_ = nullptr;
    NodeId selected_ = kInvalidNodeId;
};

}
