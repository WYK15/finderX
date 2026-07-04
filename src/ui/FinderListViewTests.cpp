#include "ui/FinderListView.h"

#include <d2d1helper.h>

#include <algorithm>
#include <cwctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace finderx;

namespace {

constexpr float kDefaultRowHeight = 21.0f;
constexpr float kTopPadding = 4.0f;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

float rowY(int index) {
    return kTopPadding + static_cast<float>(index) * kDefaultRowHeight + 1.0f;
}

float rowY(int index, float rowHeight) {
    return kTopPadding + static_cast<float>(index) * rowHeight + 1.0f;
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

std::wstring lowercase(std::wstring_view text) {
    std::wstring lowered;
    lowered.reserve(text.size());
    for (wchar_t ch : text) {
        lowered.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }
    return lowered;
}

std::vector<NodeId> visibleRowsMatchingName(const FileTree& tree, std::span<const VisibleRow> rows, std::wstring_view query) {
    const std::wstring loweredQuery = lowercase(query);
    std::vector<NodeId> matches;
    for (const VisibleRow& row : rows) {
        const std::wstring loweredName = lowercase(tree.node(row.nodeId).name);
        if (loweredName.find(loweredQuery) != std::wstring::npos) {
            matches.push_back(row.nodeId);
        }
    }
    return matches;
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

        const D2D1_RECT_F selectionRect = D2D1::RectF(10.0f, rowY(1), 500.0f, rowY(3) + 8.0f);
        require(view.selectNodesIntersecting(selectionRect, bounds, false), "rubber band should change selection");
        const NodeId expected[] = {rows[1].nodeId, rows[2].nodeId, rows[3].nodeId};
        requireSelectedNodes(view, expected, "rubber band should select intersecting visible rows");
        require(view.selectedNode() == rows[3].nodeId, "rubber band should focus last intersecting row");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.selectNode(rows[0].nodeId);
        const D2D1_RECT_F selectionRect = D2D1::RectF(10.0f, rowY(2), 500.0f, rowY(3) + 8.0f);
        require(view.selectNodesIntersecting(selectionRect, bounds, true), "additive rubber band should change selection");
        const NodeId expected[] = {rows[0].nodeId, rows[2].nodeId, rows[3].nodeId};
        requireSelectedNodes(view, expected, "additive rubber band should preserve existing selection");
    }

    {
        FileTree tree(L"C:\\Root", L"Root");
        FileNode emptyFolder;
        emptyFolder.name = L"Empty";
        emptyFolder.path = L"C:\\Root\\Empty";
        emptyFolder.kind = FileKind::Folder;
        tree.replaceChildren(tree.rootId(), {emptyFolder});
        const NodeId emptyId = tree.node(tree.rootId()).children.front();
        tree.replaceChildren(emptyId, {});

        FinderListView view(&tree);
        const D2D1_RECT_F emptyBounds = D2D1::RectF(0.0f, 0.0f, 800.0f, 400.0f);

        const ListInteractionResult result = view.onMouseDown(8.0f, 10.0f, emptyBounds, false, false);

        require(result.expandedFolder == kInvalidNodeId, "Known-empty folder disclosure click should not request expansion");
        require(!tree.node(emptyId).expanded, "Known-empty folder disclosure click should not expand folder");

        const ListInteractionResult disclosureResult = view.onMouseDown(38.0f, 10.0f, emptyBounds, false, false);

        require(disclosureResult.expandedFolder == kInvalidNodeId, "Known-empty folder disclosure hit should not request expansion");
        require(!tree.node(emptyId).expanded, "Known-empty folder disclosure hit should not expand folder");
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
        const D2D1_RECT_F nameRect = view.nameEditRectForNode(rows[2].nodeId, bounds);

        require(nameRect.left > 40.0f, "name edit rect should begin after disclosure and icon");
        require(nameRect.right > nameRect.left + 80.0f, "name edit rect should provide usable edit width");
        require(nameRect.top > rowY(1), "name edit rect should align to selected row top");
        require(nameRect.bottom > nameRect.top, "name edit rect should have positive height");

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

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        view.setStyle(ListViewStyle{18.0f, 24.0f});
        const ListInteractionResult result = view.onMouseDown(200.0f, rowY(2, 30.0f), bounds);

        require(result.changed, "large style click should change selection");
        require(view.selectedNode() == rows[2].nodeId, "large style click should hit row using dynamic row height");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);

        view.setStyle(ListViewStyle{13.0f, 14.0f, ThemeMode::Light});
        require(view.style().themeMode == ThemeMode::Light, "list view style should keep light theme");
        view.setStyle(ListViewStyle{13.0f, 14.0f, ThemeMode::Dark});
        require(view.style().themeMode == ThemeMode::Dark, "list view style should keep dark theme");
    }

    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::vector<VisibleRow> rows = tree.flatten();

        require(!view.hasFilter(), "empty filter should not be active");
        require(view.filterText().empty(), "default filter text should be empty");

        view.selectAllVisible();
        std::vector<NodeId> expectedAll;
        for (const VisibleRow& row : rows) {
            expectedAll.push_back(row.nodeId);
        }
        requireSelectedNodes(view, expectedAll, "selectAllVisible should select all rows with empty filter");

        const std::wstring sampleName = tree.node(rows[0].nodeId).name;
        std::wstring query = sampleName.substr(0, (std::min)(std::size_t{4}, sampleName.size()));
        std::transform(query.begin(), query.end(), query.begin(), [](wchar_t ch) {
            return static_cast<wchar_t>(std::towupper(ch));
        });
        const std::vector<NodeId> expectedFiltered = visibleRowsMatchingName(tree, rows, query);
        require(!expectedFiltered.empty(), "sample query should match at least one visible row");

        view.setFilterText(query);
        require(view.hasFilter(), "non-empty filter should be active");
        require(view.filterText() == query, "filterText should return the original filter text");
        view.selectAllVisible();
        requireSelectedNodes(view, expectedFiltered, "case-insensitive filter should constrain selectAllVisible");

        view.setFilterText(L"definitely-not-a-visible-sample-name");
        require(!view.hasSelection(), "unmatched filter should prune visible selection");
        const std::vector<NodeId> emptyExpected;
        require(!view.selectAllVisible(), "selectAllVisible should not change an empty unmatched selection");
        requireSelectedNodes(view, emptyExpected, "selectAllVisible should keep unmatched filter selection empty");

        view.setFilterText(L"");
        require(!view.hasFilter(), "clearing filter should make filter inactive");
        view.selectAllVisible();
        requireSelectedNodes(view, expectedAll, "clearing filter should restore all visible rows");
    }

    std::cout << "FinderListViewTests passed\n";
    return 0;
}
