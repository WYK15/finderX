#include "ui/FinderListView.h"

#include <string>

namespace finderx {

namespace {

constexpr float kRowHeight = 20.0f;

}

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {}

void FinderListView::draw(RenderContext& render, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return;
    }

    const auto rows = tree_->flatten();
    if (selected_ == kInvalidNodeId && !rows.empty()) {
        selected_ = rows.size() > 3 ? rows[3].nodeId : rows.front().nodeId;
    }

    const float dateX = bounds.right - 390.0f;
    const float sizeX = bounds.right - 230.0f;
    const float kindX = bounds.right - 130.0f;
    float y = bounds.top + 4.0f;

    for (std::size_t i = 0; i < rows.size() && y + kRowHeight <= bounds.bottom; ++i) {
        const VisibleRow& visible = rows[i];
        const FileNode& node = tree_->node(visible.nodeId);
        const bool selected = node.id == selected_;
        const D2D1_RECT_F rowRect = D2D1::RectF(bounds.left + 10.0f, y, bounds.right - 10.0f, y + kRowHeight);

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

        render.drawText(name, D2D1::RectF(nameX, y + 2.0f, dateX - 8.0f, y + kRowHeight + 2.0f), render.textFormat(), textColor);
        render.drawText(node.modified, D2D1::RectF(dateX, y + 2.0f, sizeX - 6.0f, y + kRowHeight + 2.0f), render.textFormat(), mutedColor);
        render.drawText(node.size, D2D1::RectF(sizeX, y + 2.0f, kindX - 6.0f, y + kRowHeight + 2.0f), render.textFormat(), mutedColor);
        render.drawText(node.kindText, D2D1::RectF(kindX, y + 2.0f, bounds.right - 12.0f, y + kRowHeight + 2.0f), render.textFormat(), mutedColor);

        y += kRowHeight;
    }
}

}
