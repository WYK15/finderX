#pragma once

#include "model/FileTree.h"
#include "ui/RenderContext.h"

#include <d2d1.h>
#include <windows.h>
#include <vector>

namespace finderx {

struct ListInteractionResult {
    bool changed = false;
    NodeId expandedFolder = kInvalidNodeId;
    NodeId activatedNode = kInvalidNodeId;
};

class FinderListView {
public:
    explicit FinderListView(FileTree* tree);

    void draw(RenderContext& render, const D2D1_RECT_F& bounds);
    ListInteractionResult onMouseDown(float x, float y, const D2D1_RECT_F& bounds);
    bool onWheel(int wheelDelta);
    ListInteractionResult onKeyDown(WPARAM key);
    NodeId selectedNode() const;
    NodeId nodeAtPoint(float x, float y, const D2D1_RECT_F& bounds);
    bool selectNode(NodeId id);

private:
    void rebuildRows();
    void ensureSelection();
    void ensureSelectionVisible();
    void clampScroll();
    float maxScroll() const;
    int hitTestRow(float x, float y, const D2D1_RECT_F& bounds) const;
    bool hitTestDisclosure(float x, const D2D1_RECT_F& bounds, const VisibleRow& row) const;
    int selectedRowIndex() const;

    FileTree* tree_ = nullptr;
    NodeId selected_ = kInvalidNodeId;
    NodeId lastClickedNode_ = kInvalidNodeId;
    DWORD lastClickTime_ = 0;
    float scrollY_ = 0.0f;
    float viewportHeight_ = 0.0f;
    std::vector<VisibleRow> rows_;
};

}
