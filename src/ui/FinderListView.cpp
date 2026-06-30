#include "ui/FinderListView.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <string_view>
#include <utility>

namespace finderx {

namespace {

constexpr float kTopPadding = 4.0f;
constexpr float kHorizontalInset = 10.0f;
constexpr float kMinColumnWidth = 20.0f;
constexpr float kDisclosureWidth = 18.0f;
constexpr float kIndentPerDepth = 18.0f;
constexpr float kNameStartOffset = 32.0f;
constexpr float kBaseIconSize = 14.0f;
constexpr float kIconTextGap = 5.0f;
constexpr float kWheelPixelsPerDelta = 40.0f;
constexpr float kMinFontSize = 11.0f;
constexpr float kMaxFontSize = 18.0f;
constexpr float kMinIconSize = 12.0f;
constexpr float kMaxIconSize = 24.0f;

bool hasArea(const D2D1_RECT_F& rect) {
    return rect.left < rect.right && rect.top < rect.bottom;
}

bool isExpandableFolder(const FileNode& node) {
    return node.kind == FileKind::Folder && (!node.childrenLoaded || !node.children.empty());
}

void drawTextIfWide(
    RenderContext& render,
    std::wstring_view text,
    const D2D1_RECT_F& rect,
    IDWriteTextFormat* format,
    D2D1_COLOR_F color) {
    if (rect.right - rect.left < kMinColumnWidth || rect.top >= rect.bottom) {
        return;
    }

    render.drawText(text, rect, format, color);
}

void drawDisclosure(RenderContext& render, const FileNode& node, float x, float y, float rowHeight, D2D1_COLOR_F color) {
    if (!isExpandableFolder(node)) {
        return;
    }

    const wchar_t* glyph = node.expanded ? L"\u25BE" : L"\u25B8";
    render.drawText(
        glyph,
        D2D1::RectF(x, y + 2.0f, x + kDisclosureWidth, y + rowHeight + 2.0f),
        render.textFormat(),
        color);
}

void drawFolderIcon(RenderContext& render, float x, float y, float size, bool selected) {
    const D2D1_COLOR_F tabColor = selected
        ? D2D1::ColorF(0.82f, 0.91f, 1.0f)
        : D2D1::ColorF(0.96f, 0.72f, 0.24f);
    const D2D1_COLOR_F bodyColor = selected
        ? D2D1::ColorF(0.72f, 0.86f, 1.0f)
        : D2D1::ColorF(1.0f, 0.80f, 0.32f);
    const float scale = size / kBaseIconSize;

    render.fillRoundedRect(
        D2D1::RoundedRect(
            D2D1::RectF(x + 1.0f * scale, y + 3.0f * scale, x + 7.0f * scale, y + 6.0f * scale),
            1.5f * scale,
            1.5f * scale),
        tabColor);
    render.fillRoundedRect(
        D2D1::RoundedRect(
            D2D1::RectF(x, y + 5.0f * scale, x + size, y + 13.0f * scale),
            2.0f * scale,
            2.0f * scale),
        bodyColor);
}

void drawFileIcon(RenderContext& render, float x, float y, float size, bool selected) {
    const D2D1_COLOR_F fillColor = selected
        ? D2D1::ColorF(0.96f, 0.99f, 1.0f)
        : D2D1::ColorF(1.0f, 1.0f, 1.0f);
    const D2D1_COLOR_F strokeColor = selected
        ? D2D1::ColorF(0.78f, 0.89f, 1.0f)
        : D2D1::ColorF(0.72f, 0.72f, 0.72f);
    const D2D1_COLOR_F lineColor = selected
        ? D2D1::ColorF(0.52f, 0.74f, 1.0f)
        : D2D1::ColorF(0.62f, 0.62f, 0.62f);
    const float scale = size / kBaseIconSize;

    const D2D1_RECT_F body = D2D1::RectF(x + 2.0f * scale, y + 1.0f * scale, x + 12.0f * scale, y + size);
    render.fillRoundedRect(D2D1::RoundedRect(body, 1.5f * scale, 1.5f * scale), fillColor);
    render.drawRoundedRect(D2D1::RoundedRect(body, 1.5f * scale, 1.5f * scale), strokeColor);
    render.drawLine(
        D2D1::Point2F(x + 4.0f * scale, y + 6.0f * scale),
        D2D1::Point2F(x + 10.0f * scale, y + 6.0f * scale),
        lineColor);
    render.drawLine(
        D2D1::Point2F(x + 4.0f * scale, y + 9.0f * scale),
        D2D1::Point2F(x + 10.0f * scale, y + 9.0f * scale),
        lineColor);
}

void drawNodeIcon(RenderContext& render, const FileNode& node, float x, float y, float size, bool selected) {
    if (node.kind == FileKind::Folder) {
        drawFolderIcon(render, x, y, size, selected);
        return;
    }

    drawFileIcon(render, x, y, size, selected);
}

std::wstring lowercase(std::wstring_view text) {
    std::wstring lowered;
    lowered.reserve(text.size());
    for (wchar_t ch : text) {
        lowered.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }
    return lowered;
}

bool nameMatchesFilter(const FileNode& node, const std::wstring& loweredFilter) {
    return loweredFilter.empty() || lowercase(node.name).find(loweredFilter) != std::wstring::npos;
}

}

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {
    rebuildRows();
    if (!rows_.empty()) {
        setSingleSelection(rows_.size() > 3 ? rows_[3].nodeId : rows_.front().nodeId);
    }
}

void FinderListView::setStyle(ListViewStyle style) {
    style.fontSize = (std::clamp)(style.fontSize, kMinFontSize, kMaxFontSize);
    style.iconSize = (std::clamp)(style.iconSize, kMinIconSize, kMaxIconSize);
    style_ = style;
    clampScroll();
}

ListViewStyle FinderListView::style() const {
    return style_;
}

NodeId FinderListView::selectedNode() const {
    return selected_;
}

std::vector<NodeId> FinderListView::selectedNodes() const {
    return selectedNodes_;
}

bool FinderListView::hasSelection() const {
    return !selectedNodes_.empty();
}

bool FinderListView::isSelected(NodeId id) const {
    return containsSelected(id);
}

NodeId FinderListView::nodeAtPoint(float x, float y, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return kInvalidNodeId;
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const int index = hitTestRow(x, y, bounds);
    if (index < 0) {
        return kInvalidNodeId;
    }

    return rows_[static_cast<std::size_t>(index)].nodeId;
}

bool FinderListView::selectNode(NodeId id) {
    if (!tree_ || id == kInvalidNodeId) {
        return false;
    }

    rebuildRows();
    ensureSelection();
    if (rowIndex(id) < 0) {
        return false;
    }

    const bool changed = selected_ != id || selectedNodes_.size() != 1 || selectedNodes_.front() != id || anchor_ != id;
    setSingleSelection(id);
    ensureSelectionVisible();
    return changed;
}

bool FinderListView::selectNodes(std::span<const NodeId> ids) {
    if (!tree_) {
        return false;
    }

    rebuildRows();
    ensureSelection();

    std::vector<NodeId> next;
    next.reserve(ids.size());
    for (const VisibleRow& row : rows_) {
        if (std::find(ids.begin(), ids.end(), row.nodeId) != ids.end()
            && std::find(next.begin(), next.end(), row.nodeId) == next.end()) {
            next.push_back(row.nodeId);
        }
    }

    const bool changed = next != selectedNodes_;
    selectedNodes_ = std::move(next);
    if (selectedNodes_.empty()) {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
        return changed;
    }

    if (!containsSelected(selected_)) {
        selected_ = selectedNodes_.front();
    }
    if (rowIndex(anchor_) < 0) {
        anchor_ = selected_;
    }
    ensureSelectionVisible();
    return changed;
}

bool FinderListView::selectAllVisible() {
    if (!tree_) {
        return false;
    }

    rebuildRows();
    ensureSelection();

    std::vector<NodeId> next;
    next.reserve(rows_.size());
    for (const VisibleRow& row : rows_) {
        next.push_back(row.nodeId);
    }

    const bool changed = next != selectedNodes_;
    selectedNodes_ = std::move(next);
    if (selectedNodes_.empty()) {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
        return changed;
    }

    if (!containsSelected(selected_)) {
        selected_ = selectedNodes_.front();
    }
    if (rowIndex(anchor_) < 0) {
        anchor_ = selected_;
    }
    ensureSelectionVisible();
    return changed;
}

bool FinderListView::clearSelection() {
    const bool changed = selected_ != kInvalidNodeId || anchor_ != kInvalidNodeId || !selectedNodes_.empty();
    selected_ = kInvalidNodeId;
    anchor_ = kInvalidNodeId;
    selectedNodes_.clear();
    return changed;
}

void FinderListView::setFilterText(std::wstring text) {
    filterText_ = std::move(text);
    rebuildRows();
    ensureSelection();
    ensureSelectionVisible();
}

const std::wstring& FinderListView::filterText() const {
    return filterText_;
}

bool FinderListView::hasFilter() const {
    return !filterText_.empty();
}

void FinderListView::rebuildRows() {
    if (!tree_) {
        rows_.clear();
        return;
    }

    std::vector<VisibleRow> flattened = tree_->flatten();
    if (filterText_.empty()) {
        rows_ = std::move(flattened);
        return;
    }

    const std::wstring loweredFilter = lowercase(filterText_);
    std::vector<VisibleRow> filtered;
    filtered.reserve(flattened.size());
    for (const VisibleRow& row : flattened) {
        if (nameMatchesFilter(tree_->node(row.nodeId), loweredFilter)) {
            filtered.push_back(row);
        }
    }
    rows_ = std::move(filtered);
}

void FinderListView::ensureSelection() {
    if (rows_.empty()) {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
        selectedNodes_.clear();
        return;
    }

    pruneSelectionToVisible();
    normalizeFocus();
}

float FinderListView::maxScroll() const {
    const float contentHeight = kTopPadding + static_cast<float>(rows_.size()) * rowHeight();
    return (std::max)(0.0f, contentHeight - viewportHeight_);
}

void FinderListView::clampScroll() {
    scrollY_ = (std::clamp)(scrollY_, 0.0f, maxScroll());
}

float FinderListView::rowHeight() const {
    return (std::max)(20.0f, (std::max)(iconSize() + 6.0f, style_.fontSize + 8.0f));
}

float FinderListView::iconSize() const {
    return (std::clamp)(style_.iconSize, kMinIconSize, kMaxIconSize);
}

int FinderListView::selectedRowIndex() const {
    return rowIndex(selected_);
}

int FinderListView::rowIndex(NodeId id) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].nodeId == id) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool FinderListView::containsSelected(NodeId id) const {
    return std::find(selectedNodes_.begin(), selectedNodes_.end(), id) != selectedNodes_.end();
}

bool FinderListView::setSingleSelection(NodeId id) {
    if (id == kInvalidNodeId || rowIndex(id) < 0) {
        return clearSelection();
    }

    const bool changed = selected_ != id || anchor_ != id || selectedNodes_.size() != 1 || selectedNodes_.front() != id;
    selected_ = id;
    anchor_ = id;
    selectedNodes_.assign(1, id);
    return changed;
}

bool FinderListView::setRangeSelection(NodeId from, NodeId to) {
    const int fromIndex = rowIndex(from);
    const int toIndex = rowIndex(to);
    if (fromIndex < 0 || toIndex < 0) {
        return setSingleSelection(to);
    }

    const int start = (std::min)(fromIndex, toIndex);
    const int end = (std::max)(fromIndex, toIndex);
    std::vector<NodeId> next;
    next.reserve(static_cast<std::size_t>(end - start + 1));
    for (int i = start; i <= end; ++i) {
        next.push_back(rows_[static_cast<std::size_t>(i)].nodeId);
    }

    const bool changed = selectedNodes_ != next || selected_ != to;
    selectedNodes_ = std::move(next);
    selected_ = to;
    return changed;
}

void FinderListView::pruneSelectionToVisible() {
    std::vector<NodeId> pruned;
    pruned.reserve(selectedNodes_.size());
    for (const VisibleRow& row : rows_) {
        if (containsSelected(row.nodeId)) {
            pruned.push_back(row.nodeId);
        }
    }
    selectedNodes_ = std::move(pruned);

    if (rowIndex(anchor_) < 0) {
        anchor_ = kInvalidNodeId;
    }
}

NodeId FinderListView::firstSelectedVisible() const {
    for (const VisibleRow& row : rows_) {
        if (containsSelected(row.nodeId)) {
            return row.nodeId;
        }
    }

    return kInvalidNodeId;
}

void FinderListView::normalizeFocus() {
    if (selectedNodes_.empty()) {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
        return;
    }

    if (!containsSelected(selected_)) {
        selected_ = firstSelectedVisible();
    }
    if (rowIndex(anchor_) < 0) {
        anchor_ = selected_;
    }
}

void FinderListView::ensureSelectionVisible() {
    const int index = selectedRowIndex();
    if (index < 0 || viewportHeight_ <= 0.0f) {
        clampScroll();
        return;
    }

    const float height = rowHeight();
    const float rowTop = kTopPadding + static_cast<float>(index) * height;
    const float rowBottom = rowTop + height;
    if (rowTop < scrollY_) {
        scrollY_ = rowTop;
    } else if (rowBottom > scrollY_ + viewportHeight_) {
        scrollY_ = rowBottom - viewportHeight_;
    }

    clampScroll();
}

int FinderListView::hitTestRow(float x, float y, const D2D1_RECT_F& bounds) const {
    if (!hasArea(bounds) || x < bounds.left || x > bounds.right || y < bounds.top || y > bounds.bottom) {
        return -1;
    }

    const float localY = y - bounds.top - kTopPadding + scrollY_;
    if (localY < 0.0f) {
        return -1;
    }

    const int index = static_cast<int>(std::floor(localY / rowHeight()));
    if (index < 0 || index >= static_cast<int>(rows_.size())) {
        return -1;
    }

    return index;
}

bool FinderListView::hitTestDisclosure(float x, const D2D1_RECT_F& bounds, const VisibleRow& row) const {
    const float disclosureLeft = bounds.left + kNameStartOffset + static_cast<float>(row.depth) * kIndentPerDepth;
    return x >= disclosureLeft && x <= disclosureLeft + kDisclosureWidth;
}

void FinderListView::draw(RenderContext& render, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return;
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    if (!hasArea(bounds)) {
        return;
    }

    const float contentLeft = bounds.left + kHorizontalInset;
    const float contentRight = bounds.right - kHorizontalInset;
    if (contentLeft >= contentRight) {
        return;
    }

    const float contentWidth = contentRight - contentLeft;
    const float dateWidth = contentWidth >= 520.0f ? 150.0f : 0.0f;
    const float sizeWidth = contentWidth >= 420.0f ? 80.0f : 0.0f;
    const float kindWidth = contentWidth >= 360.0f ? 120.0f : 0.0f;
    const float kindX = contentRight - kindWidth;
    const float sizeX = kindX - sizeWidth;
    const float dateX = sizeX - dateWidth;
    const float trailingLeft = dateWidth > 0.0f
        ? dateX
        : (sizeWidth > 0.0f ? sizeX : (kindWidth > 0.0f ? kindX : contentRight));
    const float nameRight = trailingLeft - 8.0f;
    const float height = rowHeight();
    const float icon = iconSize();
    float y = bounds.top + kTopPadding - scrollY_;
    ID2D1HwndRenderTarget* target = render.target();
    if (target) {
        target->PushAxisAlignedClip(bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    }

    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const VisibleRow& visible = rows_[i];
        if (y >= bounds.bottom) {
            break;
        }
        if (y + height <= bounds.top) {
            y += height;
            continue;
        }

        const FileNode& node = tree_->node(visible.nodeId);
        const bool selected = containsSelected(node.id);
        const D2D1_RECT_F rowRect = D2D1::RectF(contentLeft, y, contentRight, y + height);

        if (selected) {
            render.fillRoundedRect(
                D2D1::RoundedRect(rowRect, 5.0f, 5.0f),
                D2D1::ColorF(0.00f, 0.42f, 0.88f));
        } else if (i % 2 == 1) {
            render.fillRoundedRect(
                D2D1::RoundedRect(rowRect, 5.0f, 5.0f),
                D2D1::ColorF(0.965f, 0.965f, 0.965f));
        }

        const D2D1_COLOR_F textColor = selected
            ? D2D1::ColorF(1.0f, 1.0f, 1.0f)
            : D2D1::ColorF(0.10f, 0.10f, 0.10f);
        const D2D1_COLOR_F mutedColor = selected
            ? D2D1::ColorF(1.0f, 1.0f, 1.0f)
            : D2D1::ColorF(0.42f, 0.42f, 0.42f);
        const float disclosureX = bounds.left + kNameStartOffset + static_cast<float>(visible.depth) * kIndentPerDepth;
        const float iconX = disclosureX + kDisclosureWidth;
        const float iconY = y + (height - icon) * 0.5f;
        const float nameX = iconX + icon + kIconTextGap;

        if (isExpandableFolder(node) && disclosureX + kDisclosureWidth <= nameRight) {
            drawDisclosure(render, node, disclosureX, y, height, mutedColor);
        }
        if (iconX + icon <= nameRight) {
            drawNodeIcon(render, node, iconX, iconY, icon, selected);
        }
        drawTextIfWide(render, node.name, D2D1::RectF(nameX, y + 2.0f, nameRight, y + height + 2.0f), render.textFormat(), textColor);
        if (dateWidth > 0.0f) {
            drawTextIfWide(render, node.modified, D2D1::RectF(dateX, y + 2.0f, sizeX - 6.0f, y + height + 2.0f), render.textFormat(), mutedColor);
        }
        if (sizeWidth > 0.0f) {
            drawTextIfWide(render, node.size, D2D1::RectF(sizeX, y + 2.0f, kindX - 6.0f, y + height + 2.0f), render.textFormat(), mutedColor);
        }
        if (kindWidth > 0.0f) {
            drawTextIfWide(render, node.kindText, D2D1::RectF(kindX, y + 2.0f, contentRight, y + height + 2.0f), render.textFormat(), mutedColor);
        }

        y += height;
    }

    if (target) {
        target->PopAxisAlignedClip();
    }
}

ListInteractionResult FinderListView::onMouseDown(float x, float y, const D2D1_RECT_F& bounds, bool controlDown, bool shiftDown) {
    ListInteractionResult result;
    if (!tree_) {
        return result;
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const int index = hitTestRow(x, y, bounds);
    if (index < 0) {
        return result;
    }

    const VisibleRow row = rows_[static_cast<std::size_t>(index)];
    FileNode& node = tree_->node(row.nodeId);
    const bool disclosureClick = isExpandableFolder(node) && hitTestDisclosure(x, bounds, row);
    if (disclosureClick) {
        if (!containsSelected(row.nodeId) || (!controlDown && !shiftDown)) {
            if (setSingleSelection(row.nodeId)) {
                ensureSelectionVisible();
                result.changed = true;
            }
        }
        tree_->toggleExpanded(row.nodeId);
        if (node.expanded) {
            result.expandedFolder = row.nodeId;
        }
        rebuildRows();
        ensureSelection();
        clampScroll();
        ensureSelectionVisible();
        result.changed = true;
        lastClickedNode_ = kInvalidNodeId;
        lastClickTime_ = 0;
        return result;
    }

    bool selectionChanged = false;
    if (shiftDown && !controlDown && anchor_ != kInvalidNodeId) {
        selectionChanged = setRangeSelection(anchor_, row.nodeId);
    } else if (controlDown) {
        if (containsSelected(row.nodeId)) {
            selectedNodes_.erase(std::remove(selectedNodes_.begin(), selectedNodes_.end(), row.nodeId), selectedNodes_.end());
            if (selectedNodes_.empty()) {
                selected_ = kInvalidNodeId;
                anchor_ = kInvalidNodeId;
            } else {
                selected_ = firstSelectedVisible();
                if (rowIndex(anchor_) < 0 || anchor_ == row.nodeId) {
                    anchor_ = selected_;
                }
            }
            selectionChanged = true;
        } else {
            selectedNodes_.push_back(row.nodeId);
            std::sort(selectedNodes_.begin(), selectedNodes_.end(), [this](NodeId lhs, NodeId rhs) {
                return rowIndex(lhs) < rowIndex(rhs);
            });
            selected_ = row.nodeId;
            anchor_ = row.nodeId;
            selectionChanged = true;
        }
    } else {
        selectionChanged = setSingleSelection(row.nodeId);
    }
    if (selectionChanged) {
        ensureSelectionVisible();
        result.changed = true;
    }

    const DWORD clickTime = GetTickCount();
    const bool doubleClick = lastClickedNode_ == row.nodeId
        && clickTime - lastClickTime_ <= GetDoubleClickTime();
    lastClickedNode_ = row.nodeId;
    lastClickTime_ = clickTime;

    if (doubleClick) {
        result.activatedNode = row.nodeId;
        result.changed = true;
    }

    return result;
}

bool FinderListView::onWheel(int wheelDelta) {
    if (!tree_ || viewportHeight_ <= 0.0f) {
        return false;
    }

    rebuildRows();
    ensureSelection();
    const float oldScroll = scrollY_;
    scrollY_ -= static_cast<float>(wheelDelta) / static_cast<float>(WHEEL_DELTA) * kWheelPixelsPerDelta;
    clampScroll();
    return oldScroll != scrollY_;
}

ListInteractionResult FinderListView::onKeyDown(WPARAM key, bool controlDown, bool shiftDown) {
    ListInteractionResult result;
    if (!tree_) {
        return result;
    }

    rebuildRows();
    ensureSelection();
    if (controlDown && key == 'A') {
        result.changed = selectAllVisible();
        return result;
    }

    const int index = selectedRowIndex();
    if (index < 0) {
        return result;
    }

    switch (key) {
    case VK_UP:
        if (index > 0) {
            const NodeId next = rows_[static_cast<std::size_t>(index - 1)].nodeId;
            if (shiftDown) {
                if (anchor_ == kInvalidNodeId) {
                    anchor_ = selected_;
                }
                result.changed = setRangeSelection(anchor_, next);
            } else {
                result.changed = setSingleSelection(next);
            }
            ensureSelectionVisible();
        }
        break;
    case VK_DOWN:
        if (index + 1 < static_cast<int>(rows_.size())) {
            const NodeId next = rows_[static_cast<std::size_t>(index + 1)].nodeId;
            if (shiftDown) {
                if (anchor_ == kInvalidNodeId) {
                    anchor_ = selected_;
                }
                result.changed = setRangeSelection(anchor_, next);
            } else {
                result.changed = setSingleSelection(next);
            }
            ensureSelectionVisible();
        }
        break;
    case VK_LEFT: {
        const FileNode& node = tree_->node(selected_);
        if (isExpandableFolder(node) && node.expanded) {
            tree_->setExpanded(selected_, false);
            rebuildRows();
            ensureSelection();
            ensureSelectionVisible();
            result.changed = true;
        }
        break;
    }
    case VK_RIGHT: {
        const FileNode& node = tree_->node(selected_);
        if (isExpandableFolder(node) && !node.expanded) {
            tree_->setExpanded(selected_, true);
            result.expandedFolder = selected_;
            rebuildRows();
            ensureSelection();
            ensureSelectionVisible();
            result.changed = true;
        }
        break;
    }
    case VK_RETURN:
        if (selected_ != kInvalidNodeId) {
            result.activatedNode = selected_;
            result.changed = true;
        }
        break;
    default:
        break;
    }

    return result;
}

}
