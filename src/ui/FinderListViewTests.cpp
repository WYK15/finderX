#include "ui/FinderListView.h"

#include <d2d1helper.h>

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace finderx;

namespace {

constexpr float kRowHeight = 20.0f;
constexpr float kTopPadding = 4.0f;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

float rowY(int index) {
    return kTopPadding + static_cast<float>(index) * kRowHeight + 1.0f;
}

void requireSelectedNodes(const FinderListView& view, std::span<const NodeId> expected, const char* message) {
    const std::vector<NodeId> actual = view.selectedNodes();
    if (actual.size() != expected.size()) {
        require(false, message);
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (actual[i] != expected[i]) {
            require(false, message);
        }
    }
}

} // namespace

int main() {
    const D2D1_RECT_F bounds = D2D1::RectF(0.0f, 0.0f, 900.0f, 600.0f);

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        require(view.selectNode(rows[1].nodeId), "selectNode should select a visible node");
        const NodeId expected[] = {rows[1].nodeId};
        requireSelectedNodes(view, expected, "selectNode should yield one selected node");
        require(view.selectedNode() == rows[1].nodeId, "selectNode should focus selected node");
        require(view.hasSelection(), "selectNode should make hasSelection true");
        require(view.isSelected(rows[1].nodeId), "selectNode should make node selected");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.onMouseDown(200.0f, rowY(0), bounds);
        view.onMouseDown(200.0f, rowY(1), bounds, true);
        const NodeId expectedAfterAdd[] = {rows[0].nodeId, rows[1].nodeId};
        requireSelectedNodes(view, expectedAfterAdd, "Ctrl+click should add a second row");
        require(view.selectedNode() == rows[1].nodeId, "Ctrl+click add should focus clicked row");

        view.onMouseDown(200.0f, rowY(1), bounds, true);
        const NodeId expectedAfterToggle[] = {rows[0].nodeId};
        requireSelectedNodes(view, expectedAfterToggle, "Ctrl+click should toggle off selected second row");
        require(view.selectedNode() == rows[0].nodeId, "Ctrl+click toggle should keep another selected row focused");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.onMouseDown(200.0f, rowY(1), bounds);
        view.onMouseDown(200.0f, rowY(4), bounds, false, true);
        const NodeId expected[] = {rows[1].nodeId, rows[2].nodeId, rows[3].nodeId, rows[4].nodeId};
        requireSelectedNodes(view, expected, "Shift+click should select a visible range");
        require(view.selectedNode() == rows[4].nodeId, "Shift+click should focus clicked row");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.selectAllVisible();
        std::vector<NodeId> expected;
        for (const VisibleRow& row : rows) {
            expected.push_back(row.nodeId);
        }
        requireSelectedNodes(view, expected, "selectAllVisible should select all visible rows");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.selectNode(rows[2].nodeId);
        view.onKeyDown(VK_DOWN, false, true);
        const NodeId expected[] = {rows[2].nodeId, rows[3].nodeId};
        requireSelectedNodes(view, expected, "Shift+Down should extend selection");
        require(view.selectedNode() == rows[3].nodeId, "Shift+Down should move focus");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.selectNode(rows[2].nodeId);
        view.onKeyDown(VK_DOWN, false, true);
        view.onKeyDown(VK_DOWN);
        const NodeId expected[] = {rows[4].nodeId};
        requireSelectedNodes(view, expected, "Down without shift should collapse selection to focused row");
        require(view.selectedNode() == rows[4].nodeId, "Down without shift should move focus");
    }

    std::cout << "FinderListViewTests passed\n";
    return 0;
}
