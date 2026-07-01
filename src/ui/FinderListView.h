#pragma once

#include "model/FileTree.h"
#include "settings/AppSettings.h"
#include "ui/RenderContext.h"

#include <d2d1.h>
#include <span>
#include <string>
#include <windows.h>
#include <vector>

namespace finderx {

struct ListInteractionResult {
    bool changed = false;
    NodeId expandedFolder = kInvalidNodeId;
    NodeId activatedNode = kInvalidNodeId;
};

struct ListViewStyle {
    float fontSize = 13.0f;
    float iconSize = 14.0f;
    ThemeMode themeMode = ThemeMode::Dark;
    float modifiedColumnWidth = kDefaultModifiedColumnWidth;
    float sizeColumnWidth = kDefaultSizeColumnWidth;
    float kindColumnWidth = kDefaultKindColumnWidth;
};

class FinderListView {
public:
    explicit FinderListView(FileTree* tree);

    void setStyle(ListViewStyle style);
    ListViewStyle style() const;
    void draw(RenderContext& render, const D2D1_RECT_F& bounds);
    ListInteractionResult onMouseDown(float x, float y, const D2D1_RECT_F& bounds, bool controlDown = false, bool shiftDown = false);
    bool onWheel(int wheelDelta);
    ListInteractionResult onKeyDown(WPARAM key, bool controlDown = false, bool shiftDown = false);
    NodeId selectedNode() const;
    std::vector<NodeId> selectedNodes() const;
    bool hasSelection() const;
    bool isSelected(NodeId id) const;
    NodeId nodeAtPoint(float x, float y, const D2D1_RECT_F& bounds);
    bool selectNode(NodeId id);
    bool selectNodes(std::span<const NodeId> ids);
    bool selectAllVisible();
    bool clearSelection();
    void setFilterText(std::wstring text);
    const std::wstring& filterText() const;
    bool hasFilter() const;

private:
    void rebuildRows();
    void ensureSelection();
    void ensureSelectionVisible();
    void clampScroll();
    float rowHeight() const;
    float iconSize() const;
    float maxScroll() const;
    int hitTestRow(float x, float y, const D2D1_RECT_F& bounds) const;
    bool hitTestDisclosure(float x, const D2D1_RECT_F& bounds, const VisibleRow& row) const;
    int selectedRowIndex() const;
    int rowIndex(NodeId id) const;
    bool containsSelected(NodeId id) const;
    bool setSingleSelection(NodeId id);
    bool setRangeSelection(NodeId from, NodeId to);
    void pruneSelectionToVisible();
    void normalizeFocus();
    NodeId firstSelectedVisible() const;

    FileTree* tree_ = nullptr;
    NodeId selected_ = kInvalidNodeId;
    NodeId anchor_ = kInvalidNodeId;
    std::vector<NodeId> selectedNodes_;
    NodeId lastClickedNode_ = kInvalidNodeId;
    DWORD lastClickTime_ = 0;
    float scrollY_ = 0.0f;
    float viewportHeight_ = 0.0f;
    ListViewStyle style_;
    std::vector<VisibleRow> rows_;
    std::wstring filterText_;
};

}
