#include "ui/FinderListView.h"

#include <string>

namespace finderx {

namespace {

constexpr float kRowHeight = 20.0f;
constexpr float kHorizontalInset = 10.0f;
constexpr float kMinColumnWidth = 20.0f;

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

}

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {
    rebuildRows();
    ensureSelection();
}

void FinderListView::rebuildRows() {
    rows_ = tree_ ? tree_->flatten() : std::vector<VisibleRow>{};
}

void FinderListView::ensureSelection() {
    if (selected_ != kInvalidNodeId || rows_.empty()) {
        return;
    }

    selected_ = rows_.size() > 3 ? rows_[3].nodeId : rows_.front().nodeId;
}

void FinderListView::draw(RenderContext& render, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return;
    }

    rebuildRows();
    ensureSelection();

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
    float y = bounds.top + 4.0f;

    for (std::size_t i = 0; i < rows_.size() && y + kRowHeight <= bounds.bottom; ++i) {
        const VisibleRow& visible = rows_[i];
        const FileNode& node = tree_->node(visible.nodeId);
        const bool selected = node.id == selected_;
        const D2D1_RECT_F rowRect = D2D1::RectF(contentLeft, y, contentRight, y + kRowHeight);

        if (selected) {
            render.fillRoundedRect(
                D2D1::RoundedRect(rowRect, 5.0f, 5.0f),
                D2D1::ColorF(0.00f, 0.43f, 0.90f));
        } else if (i % 2 == 1) {
            render.fillRoundedRect(
                D2D1::RoundedRect(rowRect, 5.0f, 5.0f),
                D2D1::ColorF(0.955f, 0.955f, 0.955f));
        }

        const D2D1_COLOR_F textColor = selected
            ? D2D1::ColorF(1.0f, 1.0f, 1.0f)
            : D2D1::ColorF(0.10f, 0.10f, 0.10f);
        const D2D1_COLOR_F mutedColor = selected
            ? D2D1::ColorF(1.0f, 1.0f, 1.0f)
            : D2D1::ColorF(0.42f, 0.42f, 0.42f);
        const float nameX = bounds.left + 32.0f + static_cast<float>(visible.depth) * 18.0f;
        const wchar_t* arrow = node.kind == FileKind::Folder ? (node.expanded ? L"\u25BE" : L"\u25B8") : L" ";
        const wchar_t* marker = node.kind == FileKind::Folder ? L"[D]" : L"[F]";
        const std::wstring name = std::wstring(arrow) + L" " + marker + L" " + node.name;

        drawTextIfWide(render, name, D2D1::RectF(nameX, y + 2.0f, nameRight, y + kRowHeight + 2.0f), render.textFormat(), textColor);
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
}

}
