#include "ui/FinderListView.h"
#include "ui/FileIconResolver.h"
#include "ui/ThemeTokens.h"

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
constexpr float kListMinFontSize = 11.0f;
constexpr float kListMaxFontSize = 18.0f;
constexpr float kListMinIconSize = 12.0f;
constexpr float kListMaxIconSize = 24.0f;

bool hasArea(const D2D1_RECT_F& rect) {
    return rect.left < rect.right && rect.top < rect.bottom;
}

bool isExpandableFolder(const FileNode& node) {
    return node.kind == FileKind::Folder && (!node.childrenLoaded || !node.children.empty());
}

D2D1_COLOR_F rowStripeColor(ThemeMode mode) {
    return themeTokens(mode).rowStripe;
}

D2D1_COLOR_F rowSelectionColor(ThemeMode mode) {
    return themeTokens(mode).rowSelected;
}

D2D1_COLOR_F primaryTextColor(ThemeMode mode, bool selected) {
    if (selected) {
        return themeTokens(mode).inkOnAccent;
    }
    return themeTokens(mode).ink;
}

D2D1_COLOR_F mutedTextColor(ThemeMode mode, bool selected) {
    if (selected) {
        return themeTokens(mode).rowSelectedMutedInk;
    }
    return themeTokens(mode).inkDull;
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
        listRowTextRect(x, y, x + kDisclosureWidth, rowHeight, kListMaxFontSize),
        render.textFormat(),
        color);
}

void drawFolderIcon(RenderContext& render, float x, float y, float size, bool selected, ThemeMode mode) {
    const ThemeTokens tokens = themeTokens(mode);
    const D2D1_COLOR_F tabColor = selected ? tokens.selectedFolderTab : tokens.folderTab;
    const D2D1_COLOR_F bodyColor = selected ? tokens.selectedFolderBody : tokens.folderBody;
    const D2D1_COLOR_F highlightColor = selected ? withAlpha(tokens.inkOnAccent, 0.36f) : tokens.folderHighlight;
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
    render.fillRoundedRect(
        D2D1::RoundedRect(
            D2D1::RectF(x + 1.5f * scale, y + 6.5f * scale, x + size - 1.5f * scale, y + 8.0f * scale),
            1.0f * scale,
            1.0f * scale),
        highlightColor);
}

struct FileIconPalette {
    D2D1_COLOR_F fill{};
    D2D1_COLOR_F stroke{};
    D2D1_COLOR_F mark{};
    D2D1_COLOR_F fold{};
};

FileIconPalette fileIconPalette(FileIconKind iconKind, bool selected, ThemeMode mode) {
    const ThemeTokens tokens = themeTokens(mode);
    if (selected) {
        return {tokens.selectedIconSurface, tokens.selectedFolderTab, tokens.selectedIconLine, tokens.selectedFolderBody};
    }

    switch (iconKind) {
    case FileIconKind::Image:
        return {D2D1::ColorF(0.12f, 0.48f, 0.35f), D2D1::ColorF(0.18f, 0.72f, 0.52f), D2D1::ColorF(0.70f, 1.0f, 0.84f), D2D1::ColorF(0.30f, 0.85f, 0.58f)};
    case FileIconKind::Video:
        return {D2D1::ColorF(0.35f, 0.18f, 0.52f), D2D1::ColorF(0.62f, 0.42f, 0.95f), D2D1::ColorF(0.90f, 0.80f, 1.0f), D2D1::ColorF(0.55f, 0.34f, 0.88f)};
    case FileIconKind::Audio:
        return {D2D1::ColorF(0.40f, 0.22f, 0.10f), D2D1::ColorF(0.94f, 0.58f, 0.24f), D2D1::ColorF(1.0f, 0.78f, 0.44f), D2D1::ColorF(0.90f, 0.42f, 0.16f)};
    case FileIconKind::Archive:
        return {D2D1::ColorF(0.36f, 0.29f, 0.10f), D2D1::ColorF(0.84f, 0.70f, 0.22f), D2D1::ColorF(1.0f, 0.88f, 0.38f), D2D1::ColorF(0.72f, 0.56f, 0.14f)};
    case FileIconKind::Code:
        return {D2D1::ColorF(0.10f, 0.28f, 0.46f), D2D1::ColorF(0.24f, 0.62f, 1.0f), D2D1::ColorF(0.68f, 0.86f, 1.0f), D2D1::ColorF(0.18f, 0.48f, 0.82f)};
    case FileIconKind::Pdf:
        return {D2D1::ColorF(0.52f, 0.12f, 0.12f), D2D1::ColorF(0.90f, 0.26f, 0.24f), D2D1::ColorF(1.0f, 0.78f, 0.76f), D2D1::ColorF(0.76f, 0.16f, 0.14f)};
    case FileIconKind::Executable:
        return {D2D1::ColorF(0.18f, 0.22f, 0.28f), D2D1::ColorF(0.56f, 0.64f, 0.76f), D2D1::ColorF(0.82f, 0.88f, 0.96f), D2D1::ColorF(0.38f, 0.46f, 0.58f)};
    case FileIconKind::Document:
    case FileIconKind::Folder:
    default:
        return {tokens.appBox, tokens.appLine, tokens.inkFaint, tokens.accentFaint};
    }
}

void drawTypeMark(RenderContext& render, FileIconKind iconKind, float x, float y, float size, D2D1_COLOR_F color) {
    const float scale = size / kBaseIconSize;
    switch (iconKind) {
    case FileIconKind::Image:
        render.drawLine(D2D1::Point2F(x + 4.0f * scale, y + 10.0f * scale), D2D1::Point2F(x + 6.6f * scale, y + 7.5f * scale), color, 1.2f);
        render.drawLine(D2D1::Point2F(x + 6.6f * scale, y + 7.5f * scale), D2D1::Point2F(x + 8.4f * scale, y + 9.3f * scale), color, 1.2f);
        render.drawLine(D2D1::Point2F(x + 8.4f * scale, y + 9.3f * scale), D2D1::Point2F(x + 10.5f * scale, y + 6.7f * scale), color, 1.2f);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 4.0f * scale, y + 4.0f * scale, x + 5.8f * scale, y + 5.8f * scale), 0.9f * scale, 0.9f * scale), color);
        break;
    case FileIconKind::Video:
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 4.2f * scale, y + 4.5f * scale, x + 5.5f * scale, y + 10.5f * scale), 0.5f * scale, 0.5f * scale), color);
        render.drawLine(D2D1::Point2F(x + 6.0f * scale, y + 4.5f * scale), D2D1::Point2F(x + 10.2f * scale, y + 7.5f * scale), color, 1.8f);
        render.drawLine(D2D1::Point2F(x + 10.2f * scale, y + 7.5f * scale), D2D1::Point2F(x + 6.0f * scale, y + 10.5f * scale), color, 1.8f);
        break;
    case FileIconKind::Audio:
        render.drawLine(D2D1::Point2F(x + 5.5f * scale, y + 4.5f * scale), D2D1::Point2F(x + 5.5f * scale, y + 10.0f * scale), color, 1.4f);
        render.drawLine(D2D1::Point2F(x + 5.5f * scale, y + 4.5f * scale), D2D1::Point2F(x + 9.8f * scale, y + 3.6f * scale), color, 1.4f);
        render.drawLine(D2D1::Point2F(x + 9.8f * scale, y + 3.6f * scale), D2D1::Point2F(x + 9.8f * scale, y + 8.6f * scale), color, 1.4f);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 3.6f * scale, y + 9.0f * scale, x + 6.6f * scale, y + 11.5f * scale), 1.2f * scale, 1.2f * scale), color);
        break;
    case FileIconKind::Archive:
        for (int i = 0; i < 4; ++i) {
            const float top = y + (3.8f + static_cast<float>(i) * 2.0f) * scale;
            render.drawLine(D2D1::Point2F(x + 4.0f * scale, top), D2D1::Point2F(x + 10.0f * scale, top), color, 1.0f);
        }
        break;
    case FileIconKind::Code:
        render.drawLine(D2D1::Point2F(x + 5.5f * scale, y + 4.5f * scale), D2D1::Point2F(x + 3.5f * scale, y + 7.3f * scale), color, 1.2f);
        render.drawLine(D2D1::Point2F(x + 3.5f * scale, y + 7.3f * scale), D2D1::Point2F(x + 5.5f * scale, y + 10.0f * scale), color, 1.2f);
        render.drawLine(D2D1::Point2F(x + 8.5f * scale, y + 4.5f * scale), D2D1::Point2F(x + 10.5f * scale, y + 7.3f * scale), color, 1.2f);
        render.drawLine(D2D1::Point2F(x + 10.5f * scale, y + 7.3f * scale), D2D1::Point2F(x + 8.5f * scale, y + 10.0f * scale), color, 1.2f);
        break;
    case FileIconKind::Pdf:
        render.drawText(L"PDF", D2D1::RectF(x + 3.0f * scale, y + 5.0f * scale, x + 12.5f * scale, y + 13.0f * scale), render.headerTextFormat(), color);
        break;
    case FileIconKind::Executable:
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 4.0f * scale, y + 4.8f * scale, x + 10.0f * scale, y + 10.8f * scale), 1.4f * scale, 1.4f * scale), withAlpha(color, 0.36f));
        render.drawLine(D2D1::Point2F(x + 7.0f * scale, y + 3.2f * scale), D2D1::Point2F(x + 7.0f * scale, y + 12.0f * scale), color, 1.1f);
        render.drawLine(D2D1::Point2F(x + 2.8f * scale, y + 7.8f * scale), D2D1::Point2F(x + 11.2f * scale, y + 7.8f * scale), color, 1.1f);
        break;
    case FileIconKind::Document:
    case FileIconKind::Folder:
    default:
        render.drawLine(D2D1::Point2F(x + 4.0f * scale, y + 6.0f * scale), D2D1::Point2F(x + 10.0f * scale, y + 6.0f * scale), color);
        render.drawLine(D2D1::Point2F(x + 4.0f * scale, y + 9.0f * scale), D2D1::Point2F(x + 10.0f * scale, y + 9.0f * scale), color);
        break;
    }
}

void drawFileIcon(RenderContext& render, const FileNode& node, float x, float y, float size, bool selected, ThemeMode mode) {
    const FileIconKind iconKind = resolveFileIconKind(node);
    const FileIconPalette palette = fileIconPalette(iconKind, selected, mode);
    const float scale = size / kBaseIconSize;

    const D2D1_RECT_F body = D2D1::RectF(x + 2.0f * scale, y + 1.0f * scale, x + 12.0f * scale, y + size);
    render.fillRoundedRect(D2D1::RoundedRect(body, 1.5f * scale, 1.5f * scale), palette.fill);
    render.drawRoundedRect(D2D1::RoundedRect(body, 1.5f * scale, 1.5f * scale), palette.stroke);
    render.fillRoundedRect(
        D2D1::RoundedRect(
            D2D1::RectF(x + 8.0f * scale, y + 1.0f * scale, x + 12.0f * scale, y + 5.0f * scale),
            1.0f * scale,
            1.0f * scale),
        palette.fold);
    drawTypeMark(render, iconKind, x, y, size, palette.mark);
}

void drawNodeIcon(RenderContext& render, const FileNode& node, float x, float y, float size, bool selected, ThemeMode mode) {
    if (node.kind == FileKind::Folder) {
        drawFolderIcon(render, x, y, size, selected, mode);
        return;
    }

    drawFileIcon(render, node, x, y, size, selected, mode);
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

D2D1_RECT_F listRowTextRect(float left, float rowTop, float right, float rowHeight, float fontSize) {
    const float textHeight = (std::min)(rowHeight, (std::max)(fontSize + 6.0f, 18.0f));
    const float top = rowTop + (rowHeight - textHeight) * 0.5f;
    return D2D1::RectF(left, top, right, top + textHeight);
}

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {
    rebuildRows();
    if (!rows_.empty()) {
        setSingleSelection(rows_.size() > 3 ? rows_[3].nodeId : rows_.front().nodeId);
    }
}

void FinderListView::setStyle(ListViewStyle style) {
    style.fontSize = (std::clamp)(style.fontSize, kListMinFontSize, kListMaxFontSize);
    style.iconSize = (std::clamp)(style.iconSize, kListMinIconSize, kListMaxIconSize);
    style.itemPadding = (std::clamp)(style.itemPadding, kMinItemPadding, kMaxItemPadding);
    style.modifiedColumnWidth = (std::clamp)(style.modifiedColumnWidth, kMinModifiedColumnWidth, kMaxModifiedColumnWidth);
    style.sizeColumnWidth = (std::clamp)(style.sizeColumnWidth, kMinSizeColumnWidth, kMaxSizeColumnWidth);
    style.kindColumnWidth = (std::clamp)(style.kindColumnWidth, kMinKindColumnWidth, kMaxKindColumnWidth);
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

bool FinderListView::isFileDragHotspot(float x, float y, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return false;
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const int index = hitTestRow(x, y, bounds);
    if (index < 0) {
        return false;
    }

    const VisibleRow& row = rows_[static_cast<std::size_t>(index)];
    const FileNode& node = tree_->node(row.nodeId);
    const float disclosureX = bounds.left + kNameStartOffset + static_cast<float>(row.depth) * kIndentPerDepth;
    const float iconX = disclosureX + kDisclosureWidth;
    const float nameX = iconX + iconSize() + kIconTextGap;
    const float estimatedNameRight = nameX + (std::min)(260.0f, 12.0f + static_cast<float>(node.name.size()) * style_.fontSize * 0.58f);
    const float hotspotRight = (std::max)(iconX + iconSize(), estimatedNameRight);
    return x >= disclosureX && x <= hotspotRight;
}

D2D1_RECT_F FinderListView::nameEditRectForNode(NodeId id, const D2D1_RECT_F& bounds) {
    if (!tree_ || id == kInvalidNodeId || !hasArea(bounds)) {
        return D2D1::RectF();
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const int index = rowIndex(id);
    if (index < 0) {
        return D2D1::RectF();
    }

    const float contentLeft = bounds.left + kHorizontalInset;
    const float contentRight = bounds.right - kHorizontalInset;
    if (contentLeft >= contentRight) {
        return D2D1::RectF();
    }

    const float contentWidth = contentRight - contentLeft;
    const float dateWidth = contentWidth >= 520.0f ? style_.modifiedColumnWidth : 0.0f;
    const float sizeWidth = contentWidth >= 420.0f ? style_.sizeColumnWidth : 0.0f;
    const float kindWidth = contentWidth >= 360.0f ? style_.kindColumnWidth : 0.0f;
    const float kindX = contentRight - kindWidth;
    const float sizeX = kindX - sizeWidth;
    const float dateX = sizeX - dateWidth;
    const float trailingLeft = dateWidth > 0.0f
        ? dateX
        : (sizeWidth > 0.0f ? sizeX : (kindWidth > 0.0f ? kindX : contentRight));
    const float nameRight = trailingLeft - 8.0f;
    const VisibleRow& row = rows_[static_cast<std::size_t>(index)];
    const float height = rowHeight();
    const float icon = iconSize();
    const float rowTop = bounds.top + kTopPadding - scrollY_ + static_cast<float>(index) * height;
    const float disclosureX = bounds.left + kNameStartOffset + static_cast<float>(row.depth) * kIndentPerDepth;
    const float iconX = disclosureX + kDisclosureWidth;
    const float nameX = iconX + icon + kIconTextGap;
    const float top = rowTop + 1.0f;
    const float bottom = rowTop + height - 1.0f;
    if (nameX >= nameRight || bottom <= top) {
        return D2D1::RectF();
    }

    return D2D1::RectF(nameX, top, nameRight, bottom);
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

bool FinderListView::selectNodesIntersecting(const D2D1_RECT_F& selectionRect, const D2D1_RECT_F& bounds, bool additive) {
    if (!tree_) {
        return false;
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const D2D1_RECT_F normalized = D2D1::RectF(
        (std::min)(selectionRect.left, selectionRect.right),
        (std::min)(selectionRect.top, selectionRect.bottom),
        (std::max)(selectionRect.left, selectionRect.right),
        (std::max)(selectionRect.top, selectionRect.bottom));

    std::vector<NodeId> next;
    if (additive) {
        next = selectedNodes_;
    }

    const float height = rowHeight();
    const float rowLeft = bounds.left + kHorizontalInset;
    const float rowRight = bounds.right - kHorizontalInset;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const float rowTop = bounds.top + kTopPadding - scrollY_ + static_cast<float>(i) * height;
        const float rowBottom = rowTop + height;
        const bool intersects = normalized.left <= rowRight
            && normalized.right >= rowLeft
            && normalized.top <= rowBottom
            && normalized.bottom >= rowTop;
        if (!intersects) {
            continue;
        }

        const NodeId id = rows_[i].nodeId;
        if (std::find(next.begin(), next.end(), id) == next.end()) {
            next.push_back(id);
        }
    }

    std::sort(next.begin(), next.end(), [this](NodeId lhs, NodeId rhs) {
        return rowIndex(lhs) < rowIndex(rhs);
    });

    const bool changed = next != selectedNodes_;
    selectedNodes_ = std::move(next);
    if (selectedNodes_.empty()) {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
        return changed;
    }

    selected_ = selectedNodes_.back();
    if (rowIndex(anchor_) < 0) {
        anchor_ = selected_;
    }
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
    return (std::max)(20.0f, (std::max)(iconSize() + 6.0f, style_.fontSize + style_.itemPadding));
}

float FinderListView::iconSize() const {
    return (std::clamp)(style_.iconSize, kListMinIconSize, kListMaxIconSize);
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
    const float dateWidth = contentWidth >= 520.0f ? style_.modifiedColumnWidth : 0.0f;
    const float sizeWidth = contentWidth >= 420.0f ? style_.sizeColumnWidth : 0.0f;
    const float kindWidth = contentWidth >= 360.0f ? style_.kindColumnWidth : 0.0f;
    const float kindX = contentRight - kindWidth;
    const float sizeX = kindX - sizeWidth;
    const float dateX = sizeX - dateWidth;
    const float trailingLeft = dateWidth > 0.0f
        ? dateX
        : (sizeWidth > 0.0f ? sizeX : (kindWidth > 0.0f ? kindX : contentRight));
    const float nameRight = trailingLeft - 8.0f;
    const float height = rowHeight();
    const float icon = iconSize();
    const ThemeTokens tokens = themeTokens(style_.themeMode);
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
                D2D1::RoundedRect(rowRect, tokens.radiusControl, tokens.radiusControl),
                tokens.rowSelected);
        } else if (i % 2 == 1) {
            render.fillRoundedRect(
                D2D1::RoundedRect(rowRect, tokens.radiusControl, tokens.radiusControl),
                tokens.rowStripe);
        }

        const D2D1_COLOR_F textColor = primaryTextColor(style_.themeMode, selected);
        const D2D1_COLOR_F mutedColor = mutedTextColor(style_.themeMode, selected);
        const float disclosureX = bounds.left + kNameStartOffset + static_cast<float>(visible.depth) * kIndentPerDepth;
        const float iconX = disclosureX + kDisclosureWidth;
        const float iconY = y + (height - icon) * 0.5f;
        const float nameX = iconX + icon + kIconTextGap;

        for (int depth = 1; depth <= visible.depth; ++depth) {
            const float guideX = bounds.left + kNameStartOffset + static_cast<float>(depth - 1) * kIndentPerDepth + 8.0f;
            render.drawLine(
                D2D1::Point2F(guideX, y + 4.0f),
                D2D1::Point2F(guideX, y + height - 4.0f),
                tokens.appLine,
                1.0f);
        }
        if (isExpandableFolder(node) && disclosureX + kDisclosureWidth <= nameRight) {
            drawDisclosure(render, node, disclosureX, y, height, mutedColor);
        }
        if (iconX + icon <= nameRight) {
            drawNodeIcon(render, node, iconX, iconY, icon, selected, style_.themeMode);
        }
        drawTextIfWide(render, node.name, listRowTextRect(nameX, y, nameRight, height, style_.fontSize), render.textFormat(), textColor);
        if (dateWidth > 0.0f) {
            drawTextIfWide(render, node.modified, listRowTextRect(dateX, y, sizeX - 6.0f, height, style_.fontSize), render.textFormat(), mutedColor);
        }
        if (sizeWidth > 0.0f) {
            drawTextIfWide(render, node.size, listRowTextRect(sizeX, y, kindX - 6.0f, height, style_.fontSize), render.textFormat(), mutedColor);
        }
        if (kindWidth > 0.0f) {
            drawTextIfWide(render, node.kindText, listRowTextRect(kindX, y, contentRight, height, style_.fontSize), render.textFormat(), mutedColor);
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
