#include "ui/FinderListView.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace finderx {

namespace {

constexpr float kRowHeight = 20.0f;
constexpr float kTopPadding = 4.0f;
constexpr float kHorizontalInset = 10.0f;
constexpr float kMinColumnWidth = 20.0f;
constexpr float kDisclosureWidth = 18.0f;
constexpr float kIndentPerDepth = 18.0f;
constexpr float kNameStartOffset = 32.0f;
constexpr float kIconSize = 14.0f;
constexpr float kIconTextGap = 5.0f;
constexpr float kWheelPixelsPerDelta = 40.0f;

bool hasArea(const D2D1_RECT_F& rect) {
    return rect.left < rect.right && rect.top < rect.bottom;
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

void drawDisclosure(RenderContext& render, const FileNode& node, float x, float y, D2D1_COLOR_F color) {
    if (node.kind != FileKind::Folder) {
        return;
    }

    const wchar_t* glyph = node.expanded ? L"\u25BE" : L"\u25B8";
    render.drawText(
        glyph,
        D2D1::RectF(x, y + 2.0f, x + kDisclosureWidth, y + kRowHeight + 2.0f),
        render.textFormat(),
        color);
}

void drawFolderIcon(RenderContext& render, float x, float y, bool selected) {
    const D2D1_COLOR_F tabColor = selected
        ? D2D1::ColorF(0.82f, 0.91f, 1.0f)
        : D2D1::ColorF(0.96f, 0.72f, 0.24f);
    const D2D1_COLOR_F bodyColor = selected
        ? D2D1::ColorF(0.72f, 0.86f, 1.0f)
        : D2D1::ColorF(1.0f, 0.80f, 0.32f);

    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(x + 1.0f, y + 3.0f, x + 7.0f, y + 6.0f), 1.5f, 1.5f),
        tabColor);
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(x, y + 5.0f, x + kIconSize, y + 13.0f), 2.0f, 2.0f),
        bodyColor);
}

void drawFileIcon(RenderContext& render, float x, float y, bool selected) {
    const D2D1_COLOR_F fillColor = selected
        ? D2D1::ColorF(0.96f, 0.99f, 1.0f)
        : D2D1::ColorF(1.0f, 1.0f, 1.0f);
    const D2D1_COLOR_F strokeColor = selected
        ? D2D1::ColorF(0.78f, 0.89f, 1.0f)
        : D2D1::ColorF(0.72f, 0.72f, 0.72f);
    const D2D1_COLOR_F lineColor = selected
        ? D2D1::ColorF(0.52f, 0.74f, 1.0f)
        : D2D1::ColorF(0.62f, 0.62f, 0.62f);

    const D2D1_RECT_F body = D2D1::RectF(x + 2.0f, y + 1.0f, x + 12.0f, y + 14.0f);
    render.fillRoundedRect(D2D1::RoundedRect(body, 1.5f, 1.5f), fillColor);
    render.drawRoundedRect(D2D1::RoundedRect(body, 1.5f, 1.5f), strokeColor);
    render.drawLine(D2D1::Point2F(x + 4.0f, y + 6.0f), D2D1::Point2F(x + 10.0f, y + 6.0f), lineColor);
    render.drawLine(D2D1::Point2F(x + 4.0f, y + 9.0f), D2D1::Point2F(x + 10.0f, y + 9.0f), lineColor);
}

void drawNodeIcon(RenderContext& render, const FileNode& node, float x, float y, bool selected) {
    if (node.kind == FileKind::Folder) {
        drawFolderIcon(render, x, y, selected);
        return;
    }

    drawFileIcon(render, x, y, selected);
}

}

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {
    rebuildRows();
    ensureSelection();
}

NodeId FinderListView::selectedNode() const {
    return selected_;
}

void FinderListView::rebuildRows() {
    rows_ = tree_ ? tree_->flatten() : std::vector<VisibleRow>{};
}

void FinderListView::ensureSelection() {
    if (rows_.empty()) {
        selected_ = kInvalidNodeId;
        return;
    }

    if (selected_ != kInvalidNodeId) {
        const auto selected = std::find_if(rows_.begin(), rows_.end(), [this](const VisibleRow& row) {
            return row.nodeId == selected_;
        });
        if (selected != rows_.end()) {
            return;
        }
    }

    selected_ = rows_.size() > 3 ? rows_[3].nodeId : rows_.front().nodeId;
}

float FinderListView::maxScroll() const {
    const float contentHeight = kTopPadding + static_cast<float>(rows_.size()) * kRowHeight;
    return (std::max)(0.0f, contentHeight - viewportHeight_);
}

void FinderListView::clampScroll() {
    scrollY_ = (std::clamp)(scrollY_, 0.0f, maxScroll());
}

int FinderListView::selectedRowIndex() const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].nodeId == selected_) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void FinderListView::ensureSelectionVisible() {
    const int index = selectedRowIndex();
    if (index < 0 || viewportHeight_ <= 0.0f) {
        clampScroll();
        return;
    }

    const float rowTop = kTopPadding + static_cast<float>(index) * kRowHeight;
    const float rowBottom = rowTop + kRowHeight;
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

    const int index = static_cast<int>(std::floor(localY / kRowHeight));
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
        if (y + kRowHeight <= bounds.top) {
            y += kRowHeight;
            continue;
        }

        const FileNode& node = tree_->node(visible.nodeId);
        const bool selected = node.id == selected_;
        const D2D1_RECT_F rowRect = D2D1::RectF(contentLeft, y, contentRight, y + kRowHeight);

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
        const float iconY = y + (kRowHeight - kIconSize) * 0.5f;
        const float nameX = iconX + kIconSize + kIconTextGap;

        if (disclosureX + kDisclosureWidth <= nameRight) {
            drawDisclosure(render, node, disclosureX, y, mutedColor);
        }
        if (iconX + kIconSize <= nameRight) {
            drawNodeIcon(render, node, iconX, iconY, selected);
        }
        drawTextIfWide(render, node.name, D2D1::RectF(nameX, y + 2.0f, nameRight, y + kRowHeight + 2.0f), render.textFormat(), textColor);
        if (dateWidth > 0.0f) {
            drawTextIfWide(render, node.modified, D2D1::RectF(dateX, y + 2.0f, sizeX - 6.0f, y + kRowHeight + 2.0f), render.textFormat(), mutedColor);
        }
        if (sizeWidth > 0.0f) {
            drawTextIfWide(render, node.size, D2D1::RectF(sizeX, y + 2.0f, kindX - 6.0f, y + kRowHeight + 2.0f), render.textFormat(), mutedColor);
        }
        if (kindWidth > 0.0f) {
            drawTextIfWide(render, node.kindText, D2D1::RectF(kindX, y + 2.0f, contentRight, y + kRowHeight + 2.0f), render.textFormat(), mutedColor);
        }

        y += kRowHeight;
    }

    if (target) {
        target->PopAxisAlignedClip();
    }
}

ListInteractionResult FinderListView::onMouseDown(float x, float y, const D2D1_RECT_F& bounds) {
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
    if (selected_ != row.nodeId) {
        selected_ = row.nodeId;
        result.changed = true;
    }

    FileNode& node = tree_->node(row.nodeId);
    if (node.kind == FileKind::Folder && hitTestDisclosure(x, bounds, row)) {
        tree_->toggleExpanded(row.nodeId);
        if (node.expanded) {
            result.expandedFolder = row.nodeId;
        }
        rebuildRows();
        ensureSelection();
        clampScroll();
        result.changed = true;
        lastClickedNode_ = kInvalidNodeId;
        lastClickTime_ = 0;
        return result;
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

ListInteractionResult FinderListView::onKeyDown(WPARAM key) {
    ListInteractionResult result;
    if (!tree_) {
        return result;
    }

    rebuildRows();
    ensureSelection();
    const int index = selectedRowIndex();
    if (index < 0) {
        return result;
    }

    switch (key) {
    case VK_UP:
        if (index > 0) {
            selected_ = rows_[static_cast<std::size_t>(index - 1)].nodeId;
            ensureSelectionVisible();
            result.changed = true;
        }
        break;
    case VK_DOWN:
        if (index + 1 < static_cast<int>(rows_.size())) {
            selected_ = rows_[static_cast<std::size_t>(index + 1)].nodeId;
            ensureSelectionVisible();
            result.changed = true;
        }
        break;
    case VK_LEFT: {
        const FileNode& node = tree_->node(selected_);
        if (node.kind == FileKind::Folder && node.expanded) {
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
        if (node.kind == FileKind::Folder && !node.expanded) {
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
