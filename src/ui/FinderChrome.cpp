#include "ui/FinderChrome.h"

#include <algorithm>
#include <string>
#include <vector>

namespace finderx {

namespace {

constexpr float kSidebarWidth = 174.0f;
constexpr float kTabStripHeight = 34.0f;
constexpr float kTabTop = 6.0f;
constexpr float kTabHeight = 28.0f;
constexpr float kTabLeftPadding = 184.0f;
constexpr float kTabWidth = 146.0f;
constexpr float kTabGap = 4.0f;
constexpr float kNewTabWidth = 30.0f;
constexpr float kToolbarHeight = 53.0f;
constexpr float kHeaderBottom = 81.0f;
constexpr float kPathbarHeight = 28.0f;
constexpr float kMinTextWidth = 20.0f;
constexpr float kMinTabDrawWidth = 48.0f;
constexpr float kMinNewTabDrawWidth = 24.0f;
constexpr float kSidebarHeaderTop = 70.0f;
constexpr float kSidebarRowStartY = 94.0f;
constexpr float kSidebarRowStep = 28.0f;
constexpr float kSidebarRowLeft = 24.0f;
constexpr float kSidebarRowRight = 164.0f;
constexpr float kSidebarTextLeft = 56.0f;
constexpr float kBackLeftOffset = 18.0f;
constexpr float kBackRightOffset = 48.0f;
constexpr float kForwardLeftOffset = 52.0f;
constexpr float kForwardRightOffset = 86.0f;
constexpr float kToolbarButtonTop = 12.0f;
constexpr float kToolbarButtonBottom = 44.0f;
constexpr float kPathMinDrawWidth = 20.0f;
constexpr float kPathMinSegmentWidth = 24.0f;
constexpr float kPathTextApproxWidth = 7.2f;
constexpr float kPathChevronWidth = 20.0f;
constexpr float kToolbarUtilityButtonWidth = 32.0f;
constexpr float kToolbarUtilityButtonGap = 6.0f;
constexpr float kToolbarUtilitySearchGap = 8.0f;

struct HeaderColumnRects {
    D2D1_RECT_F name{};
    D2D1_RECT_F modified{};
    D2D1_RECT_F size{};
    D2D1_RECT_F kind{};
};

D2D1_COLOR_F rgb(float value) {
    return D2D1::ColorF(value, value, value);
}

float clampNonNegative(float value) {
    return (std::max)(0.0f, value);
}

bool hasArea(const D2D1_RECT_F& rect) {
    return rect.left < rect.right && rect.top < rect.bottom;
}

bool containsPoint(const D2D1_RECT_F& rect, float x, float y) {
    return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
}

D2D1_RECT_F clampRect(const D2D1_RECT_F& rect, const D2D1_RECT_F& bounds) {
    return D2D1::RectF(
        (std::clamp)(rect.left, bounds.left, bounds.right),
        (std::clamp)(rect.top, bounds.top, bounds.bottom),
        (std::clamp)(rect.right, bounds.left, bounds.right),
        (std::clamp)(rect.bottom, bounds.top, bounds.bottom));
}

D2D1_RECT_F searchFieldRect(const D2D1_RECT_F& toolbar) {
    if (toolbar.right - toolbar.left < 240.0f) {
        return D2D1::RectF();
    }
    const float searchLeft = (std::max)(toolbar.left + 420.0f, toolbar.right - 202.0f);
    return clampRect(D2D1::RectF(searchLeft, toolbar.top + 16.0f, toolbar.right - 14.0f, toolbar.top + 44.0f), toolbar);
}

D2D1_RECT_F settingsButtonRect(const D2D1_RECT_F& toolbar) {
    const D2D1_RECT_F searchRect = searchFieldRect(toolbar);
    if (searchRect.right - searchRect.left < 80.0f) {
        return D2D1::RectF();
    }

    const float right = searchRect.left - kToolbarUtilitySearchGap;
    const float left = right - kToolbarUtilityButtonWidth;
    if (left < toolbar.left + 250.0f) {
        return D2D1::RectF();
    }

    return D2D1::RectF(left, toolbar.top + kToolbarButtonTop, right, toolbar.top + kToolbarButtonBottom);
}

D2D1_RECT_F sortButtonRect(const D2D1_RECT_F& toolbar) {
    const D2D1_RECT_F settingsRect = settingsButtonRect(toolbar);
    if (!hasArea(settingsRect)) {
        return D2D1::RectF();
    }

    const float right = settingsRect.left - kToolbarUtilityButtonGap;
    const float left = right - kToolbarUtilityButtonWidth;
    if (left < toolbar.left + 250.0f) {
        return D2D1::RectF();
    }

    return D2D1::RectF(left, toolbar.top + kToolbarButtonTop, right, toolbar.top + kToolbarButtonBottom);
}

HeaderColumnRects headerColumnRects(const D2D1_RECT_F& header) {
    HeaderColumnRects columns;
    const float headerWidth = header.right - header.left;
    if (!hasArea(header) || headerWidth < 80.0f) {
        return columns;
    }

    const float padding = 12.0f;
    const float dateWidth = headerWidth >= 520.0f ? 150.0f : 0.0f;
    const float sizeWidth = headerWidth >= 420.0f ? 80.0f : 0.0f;
    const float kindWidth = headerWidth >= 360.0f ? 120.0f : 0.0f;
    const float kindX = header.right - padding - kindWidth;
    const float sizeX = kindX - sizeWidth;
    const float dateX = sizeX - dateWidth;
    const float trailingLeft = dateWidth > 0.0f
        ? dateX
        : (sizeWidth > 0.0f ? sizeX : (kindWidth > 0.0f ? kindX : header.right - padding));
    const float nameRight = trailingLeft - 8.0f;
    const float labelTop = header.top + 6.0f;
    const float labelBottom = header.bottom - 2.0f;

    columns.name = D2D1::RectF(header.left + 64.0f, labelTop, nameRight, labelBottom);
    if (dateWidth > 0.0f) {
        columns.modified = D2D1::RectF(dateX, labelTop, sizeX - 8.0f, labelBottom);
    }
    if (sizeWidth > 0.0f) {
        columns.size = D2D1::RectF(sizeX, labelTop, kindX - 8.0f, labelBottom);
    }
    if (kindWidth > 0.0f) {
        columns.kind = D2D1::RectF(kindX, labelTop, header.right - padding, labelBottom);
    }
    return columns;
}

D2D1_RECT_F tabRect(std::size_t index, float right) {
    const float left = kTabLeftPadding + (static_cast<float>(index) * (kTabWidth + kTabGap));
    if (left >= right) {
        return D2D1::RectF(right, kTabTop, right, kTabTop + kTabHeight);
    }

    return D2D1::RectF(left, kTabTop, (std::min)(left + kTabWidth, right), kTabTop + kTabHeight);
}

D2D1_RECT_F newTabRect(std::size_t tabCount, float right) {
    const float left = kTabLeftPadding + (static_cast<float>(tabCount) * (kTabWidth + kTabGap));
    if (left >= right) {
        return D2D1::RectF(right, kTabTop, right, kTabTop + kTabHeight);
    }

    return D2D1::RectF(left, kTabTop, (std::min)(left + kNewTabWidth, right), kTabTop + kTabHeight);
}

bool isTabRectUsable(const D2D1_RECT_F& rect) {
    return rect.right - rect.left >= kMinTabDrawWidth && rect.top < rect.bottom;
}

bool isNewTabRectUsable(const D2D1_RECT_F& rect) {
    return rect.right - rect.left >= kMinNewTabDrawWidth && rect.top < rect.bottom;
}

void drawTextClipped(
    RenderContext& render,
    std::wstring_view text,
    D2D1_RECT_F rect,
    const D2D1_RECT_F& bounds,
    IDWriteTextFormat* format,
    D2D1_COLOR_F color) {
    rect = clampRect(rect, bounds);
    if (rect.right - rect.left < kMinTextWidth || rect.top >= rect.bottom) {
        return;
    }

    render.drawText(text, rect, format, color);
}

void drawSeparator(RenderContext& render, float x, float top, float bottom) {
    if (top >= bottom) {
        return;
    }

    render.drawLine(
        D2D1::Point2F(x, top),
        D2D1::Point2F(x, bottom),
        D2D1::ColorF(0.82f, 0.82f, 0.82f));
}

std::wstring sortedHeaderLabel(std::wstring label, SortColumn column, const ChromeState& state) {
    if (state.sortColumn != column) {
        return label;
    }

    label += state.sortDirection == SortDirection::Ascending ? L" \u25B2" : L" \u25BC";
    return label;
}

D2D1_COLOR_F navigationColor(bool enabled) {
    return enabled ? D2D1::ColorF(0.20f, 0.20f, 0.20f) : D2D1::ColorF(0.68f, 0.68f, 0.68f);
}

void drawFolderGlyph(RenderContext& render, float x, float y, D2D1_COLOR_F color) {
    render.drawLine(D2D1::Point2F(x + 1.0f, y + 7.0f), D2D1::Point2F(x + 4.5f, y + 4.0f), color, 1.4f);
    render.drawLine(D2D1::Point2F(x + 4.5f, y + 4.0f), D2D1::Point2F(x + 8.0f, y + 4.0f), color, 1.4f);
    render.drawLine(D2D1::Point2F(x + 8.0f, y + 4.0f), D2D1::Point2F(x + 10.0f, y + 6.0f), color, 1.4f);
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(x + 1.0f, y + 7.0f, x + 15.0f, y + 14.0f), 1.8f, 1.8f),
        D2D1::ColorF(color.r, color.g, color.b, 0.24f));
    render.drawRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(x + 1.0f, y + 7.0f, x + 15.0f, y + 14.0f), 1.8f, 1.8f),
        color,
        1.2f);
}

void drawSidebarIcon(RenderContext& render, std::wstring_view label, float x, float y, bool selected, bool available) {
    const D2D1_COLOR_F color = !available
        ? D2D1::ColorF(0.62f, 0.62f, 0.62f)
        : (selected ? D2D1::ColorF(0.02f, 0.33f, 0.70f) : D2D1::ColorF(0.02f, 0.45f, 0.92f));

    if (label == L"Downloads") {
        render.drawLine(D2D1::Point2F(x + 8.0f, y + 2.0f), D2D1::Point2F(x + 8.0f, y + 10.0f), color, 1.6f);
        render.drawLine(D2D1::Point2F(x + 4.5f, y + 7.0f), D2D1::Point2F(x + 8.0f, y + 10.5f), color, 1.6f);
        render.drawLine(D2D1::Point2F(x + 11.5f, y + 7.0f), D2D1::Point2F(x + 8.0f, y + 10.5f), color, 1.6f);
        render.drawLine(D2D1::Point2F(x + 3.0f, y + 13.0f), D2D1::Point2F(x + 13.0f, y + 13.0f), color, 1.6f);
        render.drawLine(D2D1::Point2F(x + 3.0f, y + 13.0f), D2D1::Point2F(x + 3.0f, y + 10.0f), color, 1.2f);
        render.drawLine(D2D1::Point2F(x + 13.0f, y + 13.0f), D2D1::Point2F(x + 13.0f, y + 10.0f), color, 1.2f);
        return;
    }

    if (label == L"Home") {
        render.drawLine(D2D1::Point2F(x + 2.0f, y + 8.0f), D2D1::Point2F(x + 8.0f, y + 3.0f), color, 1.5f);
        render.drawLine(D2D1::Point2F(x + 8.0f, y + 3.0f), D2D1::Point2F(x + 14.0f, y + 8.0f), color, 1.5f);
        render.drawRoundedRect(
            D2D1::RoundedRect(D2D1::RectF(x + 4.0f, y + 8.0f, x + 12.0f, y + 14.0f), 1.6f, 1.6f),
            color,
            1.3f);
        render.drawLine(D2D1::Point2F(x + 8.0f, y + 10.0f), D2D1::Point2F(x + 8.0f, y + 14.0f), color, 1.1f);
        return;
    }

    if (label == L"Applications") {
        for (int row = 0; row < 2; ++row) {
            for (int column = 0; column < 2; ++column) {
                const float left = x + 2.0f + static_cast<float>(column) * 6.5f;
                const float top = y + 3.0f + static_cast<float>(row) * 6.5f;
                render.fillRoundedRect(
                    D2D1::RoundedRect(D2D1::RectF(left, top, left + 4.5f, top + 4.5f), 1.2f, 1.2f),
                    color);
            }
        }
        return;
    }

    if (label == L"This PC") {
        render.drawRoundedRect(
            D2D1::RoundedRect(D2D1::RectF(x + 2.0f, y + 3.0f, x + 14.0f, y + 11.0f), 1.4f, 1.4f),
            color,
            1.3f);
        render.drawLine(D2D1::Point2F(x + 6.0f, y + 12.0f), D2D1::Point2F(x + 10.0f, y + 12.0f), color, 1.3f);
        render.drawLine(D2D1::Point2F(x + 8.0f, y + 11.0f), D2D1::Point2F(x + 8.0f, y + 14.0f), color, 1.3f);
        render.drawLine(D2D1::Point2F(x + 4.5f, y + 14.0f), D2D1::Point2F(x + 11.5f, y + 14.0f), color, 1.3f);
        return;
    }

    drawFolderGlyph(render, x, y, color);
}

std::vector<std::wstring> splitPathSegments(std::wstring_view path) {
    std::vector<std::wstring> segments;
    std::size_t start = 0;

    for (std::size_t index = 0; index <= path.size(); ++index) {
        if (index == path.size() || path[index] == L'\\' || path[index] == L'/') {
            if (index > start) {
                segments.emplace_back(path.substr(start, index - start));
            }
            start = index + 1;
        }
    }

    return segments;
}

float approximateTextWidth(std::wstring_view text) {
    return static_cast<float>(text.size()) * kPathTextApproxWidth;
}

void drawPathSegments(RenderContext& render, const D2D1_RECT_F& rect, std::wstring_view path) {
    const std::vector<std::wstring> segments = splitPathSegments(path);
    const float maxRight = rect.right - 12.0f;
    float cursor = rect.left;
    const D2D1_COLOR_F textColor = D2D1::ColorF(0.34f, 0.34f, 0.34f);
    const D2D1_COLOR_F chevronColor = D2D1::ColorF(0.56f, 0.56f, 0.56f);

    if (segments.empty() || maxRight - cursor < kPathMinSegmentWidth) {
        drawTextClipped(render, path, rect, rect, render.textFormat(), textColor);
        return;
    }

    for (std::size_t index = 0; index < segments.size(); ++index) {
        const float remaining = maxRight - cursor;
        if (remaining < kPathMinDrawWidth) {
            if (index == 0) {
                drawTextClipped(render, path, rect, rect, render.textFormat(), textColor);
            }
            break;
        }

        const float preferredSegmentWidth = (std::max)(kPathMinSegmentWidth, approximateTextWidth(segments[index]));
        const float segmentWidth = (std::min)(preferredSegmentWidth, remaining);
        if (index == 0 && segmentWidth < kPathMinSegmentWidth) {
            drawTextClipped(render, path, rect, rect, render.textFormat(), textColor);
            break;
        }

        drawTextClipped(
            render,
            segments[index],
            D2D1::RectF(cursor, rect.top, cursor + segmentWidth, rect.bottom),
            rect,
            render.textFormat(),
            textColor);
        cursor += segmentWidth;

        if (index + 1 < segments.size()) {
            if (maxRight - cursor < kPathChevronWidth) {
                break;
            }
            drawTextClipped(
                render,
                L"\u203a",
                D2D1::RectF(cursor, rect.top, cursor + kPathChevronWidth, rect.bottom),
                rect,
                render.textFormat(),
                chevronColor);
            cursor += kPathChevronWidth;
        }
    }
}

const ChromeState& defaultChromeState() {
    static const ChromeState state{
        L"home",
        L"Macintosh HD > Users > leo > home > environments > androidEnv > platform-tools",
        L"",
        false,
        false,
        {
            {L"Applications", L"", true, false},
            {L"Documents", L"", true, false},
            {L"Desktop", L"", true, false},
            {L"Home", L"", true, true},
            {L"Downloads", L"", true, false},
        }};
    return state;
}

}

LayoutRects FinderChrome::layout(float width, float height) const {
    const float right = clampNonNegative(width);
    const float bottom = clampNonNegative(height);
    const float contentLeft = (std::min)(kSidebarWidth, right);
    const float tabBottom = (std::min)(kTabStripHeight, bottom);
    const float toolbarBottom = (std::min)(kTabStripHeight + kToolbarHeight, bottom);
    const float headerBottom = (std::clamp)(kTabStripHeight + kHeaderBottom, toolbarBottom, bottom);
    const float pathbarTop = (std::clamp)(bottom - kPathbarHeight, headerBottom, bottom);

    LayoutRects rects;
    rects.sidebar = D2D1::RectF(0.0f, 0.0f, contentLeft, bottom);
    rects.toolbar = D2D1::RectF(contentLeft, tabBottom, right, toolbarBottom);
    rects.header = D2D1::RectF(contentLeft, toolbarBottom, right, headerBottom);
    rects.pathbar = D2D1::RectF(contentLeft, pathbarTop, right, bottom);
    rects.list = D2D1::RectF(contentLeft, headerBottom, right, pathbarTop);
    return rects;
}

void FinderChrome::draw(RenderContext& render, const LayoutRects& rects) {
    draw(render, rects, defaultChromeState());
}

void FinderChrome::draw(RenderContext& render, const LayoutRects& rects, const ChromeState& state) {
    const D2D1_RECT_F window = D2D1::RectF(0.0f, 0.0f, rects.pathbar.right, rects.sidebar.bottom);
    const std::wstring_view title = state.title.empty() ? std::wstring_view(L"FinderX") : std::wstring_view(state.title);

    render.fillRect(rects.sidebar, D2D1::ColorF(0.92f, 0.93f, 0.94f));
    render.fillRect(rects.toolbar, D2D1::ColorF(0.97f, 0.97f, 0.965f));
    render.fillRect(rects.header, D2D1::ColorF(0.995f, 0.995f, 0.995f));
    render.fillRect(rects.pathbar, D2D1::ColorF(0.975f, 0.975f, 0.972f));
    render.fillRect(
        D2D1::RectF(0.0f, 0.0f, rects.pathbar.right, (std::min)(kTabStripHeight, rects.sidebar.bottom)),
        D2D1::ColorF(0.925f, 0.93f, 0.935f));
    render.drawLine(
        D2D1::Point2F(0.0f, kTabStripHeight),
        D2D1::Point2F(rects.pathbar.right, kTabStripHeight),
        rgb(0.78f));

    std::size_t visibleTabCount = 0;
    for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
        const D2D1_RECT_F rect = tabRect(index, rects.pathbar.right);
        if (!isTabRectUsable(rect)) {
            break;
        }
        visibleTabCount = index + 1;

        const bool active = index == state.activeTabIndex;
        const D2D1_COLOR_F fill = active
            ? D2D1::ColorF(0.985f, 0.985f, 0.982f)
            : D2D1::ColorF(0.86f, 0.865f, 0.87f);
        const D2D1_COLOR_F stroke = active
            ? D2D1::ColorF(0.76f, 0.76f, 0.76f)
            : D2D1::ColorF(0.78f, 0.785f, 0.79f);

        render.fillRoundedRect(D2D1::RoundedRect(rect, 6.0f, 6.0f), fill);
        render.drawRoundedRect(D2D1::RoundedRect(rect, 6.0f, 6.0f), stroke, 1.0f);
        drawTextClipped(
            render,
            state.tabTitles[index],
            D2D1::RectF(rect.left + 12.0f, rect.top + 5.0f, rect.right - 10.0f, rect.bottom - 3.0f),
            rect,
            render.textFormat(),
            active ? D2D1::ColorF(0.12f, 0.12f, 0.12f) : D2D1::ColorF(0.42f, 0.42f, 0.42f));
    }

    const D2D1_RECT_F plusRect = newTabRect(visibleTabCount, rects.pathbar.right);
    if (isNewTabRectUsable(plusRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(plusRect, 6.0f, 6.0f),
            D2D1::ColorF(0.875f, 0.88f, 0.885f));
        render.drawRoundedRect(
            D2D1::RoundedRect(plusRect, 6.0f, 6.0f),
            D2D1::ColorF(0.77f, 0.775f, 0.78f),
            1.0f);
        drawTextClipped(
            render,
            L"+",
            D2D1::RectF(plusRect.left + 10.0f, plusRect.top + 4.0f, plusRect.right, plusRect.bottom),
            plusRect,
            render.headerTextFormat(),
            D2D1::ColorF(0.34f, 0.34f, 0.34f));
    }

    if (rects.sidebar.right < rects.pathbar.right) {
        drawSeparator(render, rects.sidebar.right, 0.0f, rects.sidebar.bottom);
    }
    render.drawLine(
        D2D1::Point2F(rects.toolbar.left, rects.toolbar.bottom),
        D2D1::Point2F(rects.toolbar.right, rects.toolbar.bottom),
        rgb(0.82f));
    render.drawLine(
        D2D1::Point2F(rects.header.left, rects.header.bottom),
        D2D1::Point2F(rects.header.right, rects.header.bottom),
        rgb(0.86f));
    render.drawLine(
        D2D1::Point2F(rects.pathbar.left, rects.pathbar.top),
        D2D1::Point2F(rects.pathbar.right, rects.pathbar.top),
        rgb(0.86f));

    drawTextClipped(
        render,
        L"FAVORITES",
        D2D1::RectF(18.0f, kSidebarHeaderTop, 150.0f, 91.0f),
        rects.sidebar,
        render.headerTextFormat(),
        D2D1::ColorF(0.50f, 0.50f, 0.50f));

    for (std::size_t index = 0; index < state.sidebarItems.size(); ++index) {
        const SidebarItem& item = state.sidebarItems[index];
        const float rowY = kSidebarRowStartY + (static_cast<float>(index) * kSidebarRowStep);

        if (item.selected) {
            render.fillRoundedRect(
                D2D1::RoundedRect(
                    clampRect(D2D1::RectF(kSidebarRowLeft, rowY - 2.0f, kSidebarRowRight, rowY + 24.0f), rects.sidebar),
                    6.0f,
                    6.0f),
                D2D1::ColorF(0.80f, 0.82f, 0.84f));
        }

        drawSidebarIcon(render, item.label, 35.0f, rowY + 4.0f, item.selected, item.available);

        drawTextClipped(
            render,
            item.label,
            D2D1::RectF(kSidebarTextLeft, rowY + 2.0f, 160.0f, rowY + 22.0f),
            rects.sidebar,
            render.textFormat(),
            item.available ? D2D1::ColorF(0.16f, 0.16f, 0.16f) : D2D1::ColorF(0.58f, 0.58f, 0.58f));
    }

    render.fillRoundedRect(
        D2D1::RoundedRect(
            clampRect(
                D2D1::RectF(rects.toolbar.left + kBackLeftOffset, rects.toolbar.top + kToolbarButtonTop, rects.toolbar.left + kBackRightOffset, rects.toolbar.top + kToolbarButtonBottom),
                rects.toolbar),
            7.0f,
            7.0f),
        state.canGoBack ? D2D1::ColorF(0.90f, 0.91f, 0.92f) : D2D1::ColorF(0.945f, 0.945f, 0.94f));
    render.fillRoundedRect(
        D2D1::RoundedRect(
            clampRect(
                D2D1::RectF(rects.toolbar.left + kForwardLeftOffset, rects.toolbar.top + kToolbarButtonTop, rects.toolbar.left + kForwardRightOffset, rects.toolbar.top + kToolbarButtonBottom),
                rects.toolbar),
            7.0f,
            7.0f),
        state.canGoForward ? D2D1::ColorF(0.90f, 0.91f, 0.92f) : D2D1::ColorF(0.945f, 0.945f, 0.94f));
    drawTextClipped(
        render,
        L"\u2039",
        D2D1::RectF(rects.toolbar.left + 22.0f, rects.toolbar.top + 15.0f, rects.toolbar.left + 44.0f, rects.toolbar.top + 43.0f),
        rects.toolbar,
        render.textFormat(),
        navigationColor(state.canGoBack));
    drawTextClipped(
        render,
        L"\u203a",
        D2D1::RectF(rects.toolbar.left + 56.0f, rects.toolbar.top + 15.0f, rects.toolbar.left + 82.0f, rects.toolbar.top + 43.0f),
        rects.toolbar,
        render.textFormat(),
        navigationColor(state.canGoForward));
    drawTextClipped(
        render,
        title,
        D2D1::RectF(rects.toolbar.left + 106.0f, rects.toolbar.top + 16.0f, rects.toolbar.left + 240.0f, rects.toolbar.top + 45.0f),
        rects.toolbar,
        render.headerTextFormat(),
        D2D1::ColorF(0.18f, 0.18f, 0.18f));
    drawTextClipped(
        render,
        L"\u25A6   \u25A4   \u25A1   \u2022\u2022\u2022",
        D2D1::RectF(rects.toolbar.left + 380.0f, rects.toolbar.top + 17.0f, rects.toolbar.right - 232.0f, rects.toolbar.top + 44.0f),
        rects.toolbar,
        render.textFormat(),
        D2D1::ColorF(0.36f, 0.36f, 0.36f));

    const D2D1_RECT_F sortRect = sortButtonRect(rects.toolbar);
    if (hasArea(sortRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(sortRect, 7.0f, 7.0f),
            D2D1::ColorF(0.91f, 0.92f, 0.93f));
        render.drawRoundedRect(
            D2D1::RoundedRect(sortRect, 7.0f, 7.0f),
            rgb(0.82f),
            1.0f);
        drawTextClipped(
            render,
            state.sortDirection == SortDirection::Ascending ? std::wstring_view(L"\u2191\u2193") : std::wstring_view(L"\u2193\u2191"),
            D2D1::RectF(sortRect.left + 7.0f, sortRect.top + 4.0f, sortRect.right, sortRect.bottom),
            sortRect,
            render.textFormat(),
            D2D1::ColorF(0.30f, 0.30f, 0.30f));
    }

    const D2D1_RECT_F settingsRect = settingsButtonRect(rects.toolbar);
    if (hasArea(settingsRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(settingsRect, 7.0f, 7.0f),
            D2D1::ColorF(0.91f, 0.92f, 0.93f));
        render.drawRoundedRect(
            D2D1::RoundedRect(settingsRect, 7.0f, 7.0f),
            rgb(0.82f),
            1.0f);
        drawTextClipped(
            render,
            L"\u2699",
            D2D1::RectF(settingsRect.left + 9.0f, settingsRect.top + 3.0f, settingsRect.right, settingsRect.bottom),
            settingsRect,
            render.textFormat(),
            D2D1::ColorF(0.30f, 0.30f, 0.30f));
    }

    const D2D1_RECT_F searchRect = searchFieldRect(rects.toolbar);
    if (searchRect.right - searchRect.left >= 80.0f) {
        const bool hasSearchText = !state.searchText.empty();
        render.fillRoundedRect(
            D2D1::RoundedRect(searchRect, 7.0f, 7.0f),
            D2D1::ColorF(1.0f, 1.0f, 1.0f));
        render.drawRoundedRect(
            D2D1::RoundedRect(searchRect, 7.0f, 7.0f),
            state.searchFocused ? D2D1::ColorF(0.10f, 0.45f, 0.92f) : rgb(0.88f),
            state.searchFocused ? 1.4f : 1.0f);
        drawTextClipped(
            render,
            hasSearchText ? std::wstring_view(state.searchText) : std::wstring_view(L"\u2315 Search"),
            D2D1::RectF(searchRect.left + 11.0f, searchRect.top + 4.0f, searchRect.right - 12.0f, searchRect.bottom),
            searchRect,
            render.textFormat(),
            hasSearchText ? D2D1::ColorF(0.18f, 0.18f, 0.18f) : D2D1::ColorF(0.53f, 0.53f, 0.53f));
    }

    const HeaderColumnRects columns = headerColumnRects(rects.header);
    if (hasArea(columns.name)) {
        const D2D1_COLOR_F activeHeader = D2D1::ColorF(0.18f, 0.18f, 0.18f);
        const D2D1_COLOR_F inactiveHeader = D2D1::ColorF(0.38f, 0.38f, 0.38f);
        drawTextClipped(render, sortedHeaderLabel(L"Name", SortColumn::Name, state), columns.name, rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Name ? activeHeader : inactiveHeader);
        if (hasArea(columns.modified)) {
            drawTextClipped(render, sortedHeaderLabel(L"Date Modified", SortColumn::Modified, state), columns.modified, rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Modified ? activeHeader : inactiveHeader);
        }
        if (hasArea(columns.size)) {
            drawTextClipped(render, sortedHeaderLabel(L"Size", SortColumn::Size, state), columns.size, rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Size ? activeHeader : inactiveHeader);
        }
        if (hasArea(columns.kind)) {
            drawTextClipped(render, sortedHeaderLabel(L"Kind", SortColumn::Kind, state), columns.kind, rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Kind ? activeHeader : inactiveHeader);
        }
    }

    const D2D1_RECT_F pathTextRect = D2D1::RectF(
        rects.pathbar.left + 14.0f,
        rects.pathbar.top + 5.0f,
        rects.pathbar.right - 12.0f,
        rects.pathbar.bottom);
    if (state.statusText.empty()) {
        drawPathSegments(render, clampRect(pathTextRect, window), state.pathText);
    } else {
        drawTextClipped(
            render,
            state.statusText,
            pathTextRect,
            window,
            render.textFormat(),
            D2D1::ColorF(0.72f, 0.18f, 0.16f));
    }
}

ChromeHitResult FinderChrome::hitTest(float x, float y, const LayoutRects& rects, const ChromeState& state) const {
    std::size_t visibleTabCount = 0;
    for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
        const D2D1_RECT_F rect = tabRect(index, rects.pathbar.right);
        if (!isTabRectUsable(rect)) {
            break;
        }
        visibleTabCount = index + 1;
        if (containsPoint(rect, x, y)) {
            return {ChromeHitKind::Tab, 0, index};
        }
    }

    const D2D1_RECT_F plusRect = newTabRect(visibleTabCount, rects.pathbar.right);
    if (isNewTabRectUsable(plusRect) && containsPoint(plusRect, x, y)) {
        return {ChromeHitKind::NewTab, 0, 0};
    }

    const D2D1_RECT_F backRect = D2D1::RectF(
        rects.toolbar.left + kBackLeftOffset,
        rects.toolbar.top + kToolbarButtonTop,
        rects.toolbar.left + kBackRightOffset,
        rects.toolbar.top + kToolbarButtonBottom);
    if (state.canGoBack && containsPoint(backRect, x, y)) {
        return {ChromeHitKind::Back, 0, 0};
    }

    const D2D1_RECT_F forwardRect = D2D1::RectF(
        rects.toolbar.left + kForwardLeftOffset,
        rects.toolbar.top + kToolbarButtonTop,
        rects.toolbar.left + kForwardRightOffset,
        rects.toolbar.top + kToolbarButtonBottom);
    if (state.canGoForward && containsPoint(forwardRect, x, y)) {
        return {ChromeHitKind::Forward, 0, 0};
    }

    const D2D1_RECT_F sortRect = sortButtonRect(rects.toolbar);
    if (hasArea(sortRect) && containsPoint(sortRect, x, y)) {
        return {ChromeHitKind::SortMenu, 0, 0};
    }

    const D2D1_RECT_F settingsRect = settingsButtonRect(rects.toolbar);
    if (hasArea(settingsRect) && containsPoint(settingsRect, x, y)) {
        return {ChromeHitKind::Settings, 0, 0};
    }

    const D2D1_RECT_F searchRect = searchFieldRect(rects.toolbar);
    if (searchRect.right - searchRect.left >= 80.0f && containsPoint(searchRect, x, y)) {
        return {ChromeHitKind::SearchField, 0, 0};
    }

    const HeaderColumnRects columns = headerColumnRects(rects.header);
    if (hasArea(columns.name) && containsPoint(columns.name, x, y)) {
        return {ChromeHitKind::HeaderName, 0, 0};
    }
    if (hasArea(columns.modified) && containsPoint(columns.modified, x, y)) {
        return {ChromeHitKind::HeaderModified, 0, 0};
    }
    if (hasArea(columns.size) && containsPoint(columns.size, x, y)) {
        return {ChromeHitKind::HeaderSize, 0, 0};
    }
    if (hasArea(columns.kind) && containsPoint(columns.kind, x, y)) {
        return {ChromeHitKind::HeaderKind, 0, 0};
    }

    for (std::size_t index = 0; index < state.sidebarItems.size(); ++index) {
        const SidebarItem& item = state.sidebarItems[index];
        const float rowY = kSidebarRowStartY + (static_cast<float>(index) * kSidebarRowStep);
        const D2D1_RECT_F rowRect = D2D1::RectF(kSidebarRowLeft, rowY - 2.0f, kSidebarRowRight, rowY + 24.0f);
        if (item.available && containsPoint(rowRect, x, y)) {
            return {ChromeHitKind::SidebarItem, index, 0};
        }
    }

    return {};
}

}
