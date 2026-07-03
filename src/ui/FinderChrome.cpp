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
constexpr float kSidebarSectionHeaderHeight = 24.0f;
constexpr float kSidebarSectionGap = 6.0f;
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
constexpr float kPathTextPadding = 18.0f;
constexpr float kPathChevronWidth = 20.0f;
constexpr float kColumnResizeHitSlop = 5.0f;
constexpr float kToolbarUtilityButtonWidth = 32.0f;
constexpr float kToolbarUtilityButtonGap = 6.0f;
constexpr float kToolbarUtilitySearchGap = 8.0f;

struct HeaderColumnRects {
    D2D1_RECT_F name{};
    D2D1_RECT_F modified{};
    D2D1_RECT_F size{};
    D2D1_RECT_F kind{};
};

struct SidebarLayoutRow {
    std::size_t index = 0;
    float rowY = 0.0f;
    float headerTop = 0.0f;
    const wchar_t* header = nullptr;
};

struct PathSegmentLayout {
    std::wstring label;
    std::wstring targetPath;
    D2D1_RECT_F rect{};
};

D2D1_COLOR_F rgb(float value) {
    return D2D1::ColorF(value, value, value);
}

bool isDark(ThemeMode mode) {
    return mode == ThemeMode::Dark;
}

D2D1_COLOR_F themeColor(ThemeMode mode, D2D1_COLOR_F light, D2D1_COLOR_F dark) {
    return isDark(mode) ? dark : light;
}

D2D1_COLOR_F withAlpha(D2D1_COLOR_F color, float alpha) {
    color.a = alpha;
    return color;
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

const wchar_t* sidebarHeaderForRole(SidebarItemRole role) {
    return role == SidebarItemRole::Location ? L"LOCATIONS" : L"FAVORITES";
}

std::vector<SidebarLayoutRow> sidebarLayoutRows(const std::vector<SidebarItem>& items) {
    std::vector<SidebarLayoutRow> rows;
    rows.reserve(items.size());

    float cursor = kSidebarHeaderTop;
    bool sawFavorite = false;
    bool sawLocation = false;
    for (std::size_t index = 0; index < items.size(); ++index) {
        const SidebarItemRole role = items[index].role;
        bool* sawSection = role == SidebarItemRole::Location ? &sawLocation : &sawFavorite;

        SidebarLayoutRow row;
        row.index = index;
        if (!*sawSection) {
            if (!rows.empty()) {
                cursor += kSidebarSectionGap;
            }
            row.headerTop = cursor;
            row.header = sidebarHeaderForRole(role);
            cursor += kSidebarSectionHeaderHeight;
            *sawSection = true;
        }

        row.rowY = cursor;
        cursor += kSidebarRowStep;
        rows.push_back(row);
    }

    return rows;
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

float visibleColumnWidth(float availableWidth, float threshold, float preferredWidth) {
    return availableWidth >= threshold ? preferredWidth : 0.0f;
}

HeaderColumnRects headerColumnRects(const D2D1_RECT_F& header, const ChromeState& state) {
    HeaderColumnRects columns;
    const float headerWidth = header.right - header.left;
    if (!hasArea(header) || headerWidth < 80.0f) {
        return columns;
    }

    const float padding = 12.0f;
    const float dateWidth = visibleColumnWidth(headerWidth, 520.0f, state.modifiedColumnWidth);
    const float sizeWidth = visibleColumnWidth(headerWidth, 420.0f, state.sizeColumnWidth);
    const float kindWidth = visibleColumnWidth(headerWidth, 360.0f, state.kindColumnWidth);
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

D2D1_RECT_F columnResizeRect(float x, const D2D1_RECT_F& header) {
    return D2D1::RectF(x - kColumnResizeHitSlop, header.top, x + kColumnResizeHitSlop, header.bottom);
}

void drawHeaderColumnSeparator(RenderContext& render, const D2D1_RECT_F& header, float x, ThemeMode mode) {
    render.drawLine(
        D2D1::Point2F(x, header.top + 5.0f),
        D2D1::Point2F(x, header.bottom - 5.0f),
        themeColor(mode, D2D1::ColorF(0.84f, 0.84f, 0.84f), D2D1::ColorF(0.26f, 0.31f, 0.40f)),
        1.0f);
}

D2D1_RECT_F tabRect(std::size_t index, float right) {
    const float left = kTabLeftPadding + (static_cast<float>(index) * (kTabWidth + kTabGap));
    if (left >= right) {
        return D2D1::RectF(right, kTabTop, right, kTabTop + kTabHeight);
    }

    return D2D1::RectF(left, kTabTop, (std::min)(left + kTabWidth, right), kTabTop + kTabHeight);
}

D2D1_RECT_F tabCloseRect(const D2D1_RECT_F& tab) {
    if (tab.right - tab.left <= 72.0f) {
        return D2D1::RectF();
    }

    return D2D1::RectF(tab.right - 28.0f, tab.top + 6.0f, tab.right - 8.0f, tab.bottom - 6.0f);
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

void drawSeparator(RenderContext& render, float x, float top, float bottom, D2D1_COLOR_F color) {
    if (top >= bottom) {
        return;
    }

    render.drawLine(
        D2D1::Point2F(x, top),
        D2D1::Point2F(x, bottom),
        color);
}

std::wstring sortedHeaderLabel(std::wstring label, SortColumn column, const ChromeState& state) {
    if (state.sortColumn != column) {
        return label;
    }

    label += state.sortDirection == SortDirection::Ascending ? L" \u25B2" : L" \u25BC";
    return label;
}

D2D1_COLOR_F navigationColor(bool enabled, ThemeMode mode) {
    if (isDark(mode)) {
        return enabled ? D2D1::ColorF(0.88f, 0.92f, 0.98f) : D2D1::ColorF(0.35f, 0.40f, 0.50f);
    }
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

void drawSidebarIconPlate(RenderContext& render, float x, float y, bool selected, bool available, ThemeMode mode) {
    const D2D1_COLOR_F fill = !available
        ? themeColor(mode, D2D1::ColorF(0.84f, 0.85f, 0.86f, 0.50f), D2D1::ColorF(0.14f, 0.16f, 0.21f, 0.70f))
        : (selected
            ? themeColor(mode, D2D1::ColorF(0.70f, 0.82f, 1.0f, 0.55f), D2D1::ColorF(0.12f, 0.30f, 0.62f, 0.90f))
            : themeColor(mode, D2D1::ColorF(0.84f, 0.90f, 1.0f, 0.55f), D2D1::ColorF(0.11f, 0.15f, 0.23f, 0.95f)));
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(x - 5.0f, y - 3.0f, x + 21.0f, y + 21.0f), 6.0f, 6.0f),
        fill);
}

void drawSidebarIcon(RenderContext& render, std::wstring_view label, float x, float y, bool selected, bool available, ThemeMode mode) {
    drawSidebarIconPlate(render, x, y, selected, available, mode);

    const D2D1_COLOR_F color = !available
        ? themeColor(mode, D2D1::ColorF(0.62f, 0.62f, 0.62f), D2D1::ColorF(0.35f, 0.40f, 0.50f))
        : (selected
            ? themeColor(mode, D2D1::ColorF(0.02f, 0.33f, 0.70f), D2D1::ColorF(0.55f, 0.72f, 1.0f))
            : themeColor(mode, D2D1::ColorF(0.02f, 0.45f, 0.92f), D2D1::ColorF(0.25f, 0.58f, 1.0f)));

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

void drawSortGlyph(RenderContext& render, const D2D1_RECT_F& rect, SortDirection direction, D2D1_COLOR_F color) {
    const float left = rect.left + 8.0f;
    const float top = rect.top + 8.0f;
    const float bottom = rect.bottom - 8.0f;
    render.drawLine(D2D1::Point2F(left, top), D2D1::Point2F(left, bottom), color, 1.4f);
    render.drawLine(D2D1::Point2F(left + 8.0f, top), D2D1::Point2F(left + 8.0f, bottom), color, 1.4f);
    if (direction == SortDirection::Ascending) {
        render.drawLine(D2D1::Point2F(left - 3.0f, top + 4.0f), D2D1::Point2F(left, top), color, 1.4f);
        render.drawLine(D2D1::Point2F(left + 3.0f, top + 4.0f), D2D1::Point2F(left, top), color, 1.4f);
        render.drawLine(D2D1::Point2F(left + 5.0f, bottom - 4.0f), D2D1::Point2F(left + 8.0f, bottom), color, 1.4f);
        render.drawLine(D2D1::Point2F(left + 11.0f, bottom - 4.0f), D2D1::Point2F(left + 8.0f, bottom), color, 1.4f);
    } else {
        render.drawLine(D2D1::Point2F(left - 3.0f, bottom - 4.0f), D2D1::Point2F(left, bottom), color, 1.4f);
        render.drawLine(D2D1::Point2F(left + 3.0f, bottom - 4.0f), D2D1::Point2F(left, bottom), color, 1.4f);
        render.drawLine(D2D1::Point2F(left + 5.0f, top + 4.0f), D2D1::Point2F(left + 8.0f, top), color, 1.4f);
        render.drawLine(D2D1::Point2F(left + 11.0f, top + 4.0f), D2D1::Point2F(left + 8.0f, top), color, 1.4f);
    }
}

void drawSettingsGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const float cx = rect.left + 16.0f;
    const float cy = rect.top + 16.0f;
    render.drawRoundedRect(D2D1::RoundedRect(D2D1::RectF(cx - 5.0f, cy - 5.0f, cx + 5.0f, cy + 5.0f), 5.0f, 5.0f), color, 1.4f);
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(cx - 1.5f, cy - 1.5f, cx + 1.5f, cy + 1.5f), 1.5f, 1.5f), color);
    for (int index = 0; index < 4; ++index) {
        const float dx = (index % 2 == 0) ? 0.0f : 7.5f;
        const float dy = (index % 2 == 0) ? 7.5f : 0.0f;
        render.drawLine(D2D1::Point2F(cx - dx, cy - dy), D2D1::Point2F(cx - dx * 0.70f, cy - dy * 0.70f), color, 1.4f);
        render.drawLine(D2D1::Point2F(cx + dx, cy + dy), D2D1::Point2F(cx + dx * 0.70f, cy + dy * 0.70f), color, 1.4f);
    }
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
    float width = kPathTextPadding;
    for (wchar_t character : text) {
        if (character <= 0x007F) {
            width += 9.8f;
        } else {
            width += 14.0f;
        }
    }
    return width;
}

float approximateInlineTextWidth(std::wstring_view text) {
    float width = 0.0f;
    for (wchar_t character : text) {
        width += character <= 0x007F ? 7.5f : 13.0f;
    }
    return width;
}

std::wstring pathTargetForSegment(const std::vector<std::wstring>& segments, std::size_t index) {
    std::wstring target;
    for (std::size_t segmentIndex = 0; segmentIndex <= index && segmentIndex < segments.size(); ++segmentIndex) {
        if (segmentIndex == 0) {
            target = segments[segmentIndex];
            if (!target.empty() && target.back() == L':') {
                target += L"\\";
            }
            continue;
        }

        if (!target.empty() && target.back() != L'\\' && target.back() != L'/') {
            target += L"\\";
        }
        target += segments[segmentIndex];
    }
    return target;
}

std::vector<PathSegmentLayout> pathSegmentLayouts(const D2D1_RECT_F& rect, std::wstring_view path) {
    const std::vector<std::wstring> segments = splitPathSegments(path);
    const float maxRight = rect.right - 12.0f;
    float cursor = rect.left;
    std::vector<PathSegmentLayout> layouts;

    if (segments.empty() || maxRight - cursor < kPathMinSegmentWidth) {
        return layouts;
    }
    if (segments.size() == 1 && (segments.front().empty() || segments.front().back() != L':')) {
        return layouts;
    }

    for (std::size_t index = 0; index < segments.size(); ++index) {
        const float remaining = maxRight - cursor;
        if (remaining < kPathMinDrawWidth) {
            break;
        }

        const float preferredSegmentWidth = (std::max)(kPathMinSegmentWidth, approximateTextWidth(segments[index]));
        const float segmentWidth = (std::min)(preferredSegmentWidth, remaining);
        if (index == 0 && segmentWidth < kPathMinSegmentWidth) {
            break;
        }

        layouts.push_back({segments[index], pathTargetForSegment(segments, index), D2D1::RectF(cursor, rect.top, cursor + segmentWidth, rect.bottom)});
        cursor += segmentWidth;

        if (index + 1 < segments.size()) {
            if (maxRight - cursor < kPathChevronWidth) {
                break;
            }
            cursor += kPathChevronWidth;
        }
    }

    return layouts;
}

void drawPathSegments(RenderContext& render, const D2D1_RECT_F& rect, std::wstring_view path, ThemeMode mode) {
    const std::vector<PathSegmentLayout> layouts = pathSegmentLayouts(rect, path);
    const D2D1_COLOR_F textColor = themeColor(mode, D2D1::ColorF(0.34f, 0.34f, 0.34f), D2D1::ColorF(0.72f, 0.78f, 0.88f));
    const D2D1_COLOR_F chevronColor = themeColor(mode, D2D1::ColorF(0.56f, 0.56f, 0.56f), D2D1::ColorF(0.42f, 0.48f, 0.58f));

    if (layouts.empty()) {
        drawTextClipped(render, path, rect, rect, render.textFormat(), textColor);
        return;
    }

    for (std::size_t index = 0; index < layouts.size(); ++index) {
        const PathSegmentLayout& segment = layouts[index];
        drawTextClipped(
            render,
            segment.label,
            segment.rect,
            rect,
            render.textFormat(),
            textColor);

        if (index + 1 < layouts.size()) {
            drawTextClipped(
                render,
                L"\u203a",
                D2D1::RectF(segment.rect.right, rect.top, segment.rect.right + kPathChevronWidth, rect.bottom),
                rect,
                render.textFormat(),
                chevronColor);
        }
    }
}

D2D1_RECT_F pathTextRect(const LayoutRects& rects) {
    return D2D1::RectF(
        rects.pathbar.left + 14.0f,
        rects.pathbar.top + 5.0f,
        rects.pathbar.right - 12.0f,
        rects.pathbar.bottom);
}

const ChromeState& defaultChromeState() {
    static const ChromeState state{
        L"home",
        L"Macintosh HD > Users > leo > home > environments > androidEnv > platform-tools",
        L"",
        false,
        L"",
        0,
        0,
        0,
        false,
        false,
        false,
        {
            {L"Applications", L"", SidebarItemRole::Favorite, true, false},
            {L"Documents", L"", SidebarItemRole::Favorite, true, false},
            {L"Desktop", L"", SidebarItemRole::Favorite, true, false},
            {L"Home", L"", SidebarItemRole::Favorite, true, true},
            {L"This PC", thisPcPath(), SidebarItemRole::Location, true, false},
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
    const ThemeMode mode = state.themeMode;
    const D2D1_COLOR_F separatorColor = themeColor(mode, rgb(0.82f), D2D1::ColorF(0.16f, 0.19f, 0.26f));
    const D2D1_COLOR_F textPrimary = themeColor(mode, D2D1::ColorF(0.18f, 0.18f, 0.18f), D2D1::ColorF(0.90f, 0.93f, 0.98f));
    const D2D1_COLOR_F textSecondary = themeColor(mode, D2D1::ColorF(0.38f, 0.38f, 0.38f), D2D1::ColorF(0.60f, 0.66f, 0.76f));
    const D2D1_COLOR_F mutedText = themeColor(mode, D2D1::ColorF(0.50f, 0.50f, 0.50f), D2D1::ColorF(0.43f, 0.49f, 0.59f));
    const D2D1_COLOR_F controlFill = themeColor(mode, D2D1::ColorF(0.91f, 0.92f, 0.93f), D2D1::ColorF(0.13f, 0.16f, 0.22f));
    const D2D1_COLOR_F controlStroke = themeColor(mode, rgb(0.82f), D2D1::ColorF(0.22f, 0.27f, 0.36f));

    render.fillRect(rects.sidebar, themeColor(mode, D2D1::ColorF(0.92f, 0.93f, 0.94f), D2D1::ColorF(0.075f, 0.090f, 0.125f)));
    render.fillRect(rects.toolbar, themeColor(mode, D2D1::ColorF(0.97f, 0.97f, 0.965f), D2D1::ColorF(0.080f, 0.100f, 0.145f)));
    render.fillRect(rects.header, themeColor(mode, D2D1::ColorF(0.995f, 0.995f, 0.995f), D2D1::ColorF(0.070f, 0.085f, 0.120f)));
    render.fillRect(rects.pathbar, themeColor(mode, D2D1::ColorF(0.975f, 0.975f, 0.972f), D2D1::ColorF(0.080f, 0.095f, 0.135f)));
    render.fillRect(
        D2D1::RectF(0.0f, 0.0f, rects.pathbar.right, (std::min)(kTabStripHeight, rects.sidebar.bottom)),
        themeColor(mode, D2D1::ColorF(0.925f, 0.93f, 0.935f), D2D1::ColorF(0.055f, 0.070f, 0.105f)));
    if (rects.sidebar.right >= 120.0f && rects.sidebar.bottom >= 62.0f) {
        const D2D1_COLOR_F logoFill = themeColor(mode, D2D1::ColorF(0.18f, 0.45f, 0.98f), D2D1::ColorF(0.18f, 0.40f, 0.92f));
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(18.0f, 22.0f, 42.0f, 46.0f), 7.0f, 7.0f), logoFill);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(24.0f, 28.0f, 36.0f, 40.0f), 3.0f, 3.0f), withAlpha(D2D1::ColorF(1.0f, 1.0f, 1.0f), 0.22f));
        drawTextClipped(
            render,
            L"FinderX",
            D2D1::RectF(52.0f, 23.0f, 158.0f, 46.0f),
            rects.sidebar,
            render.headerTextFormat(),
            textPrimary);
    }
    render.drawLine(
        D2D1::Point2F(rects.sidebar.right, kTabStripHeight),
        D2D1::Point2F(rects.pathbar.right, kTabStripHeight),
        separatorColor);

    std::size_t visibleTabCount = 0;
    for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
        const D2D1_RECT_F rect = tabRect(index, rects.pathbar.right);
        if (!isTabRectUsable(rect)) {
            break;
        }
        visibleTabCount = index + 1;

        const bool active = index == state.activeTabIndex;
        const D2D1_COLOR_F fill = active
            ? themeColor(mode, D2D1::ColorF(0.985f, 0.985f, 0.982f), D2D1::ColorF(0.105f, 0.130f, 0.190f))
            : themeColor(mode, D2D1::ColorF(0.86f, 0.865f, 0.87f), D2D1::ColorF(0.075f, 0.090f, 0.130f));
        const D2D1_COLOR_F stroke = active
            ? themeColor(mode, D2D1::ColorF(0.76f, 0.76f, 0.76f), D2D1::ColorF(0.25f, 0.33f, 0.48f))
            : themeColor(mode, D2D1::ColorF(0.78f, 0.785f, 0.79f), D2D1::ColorF(0.16f, 0.19f, 0.26f));

        render.fillRoundedRect(D2D1::RoundedRect(rect, 6.0f, 6.0f), fill);
        render.drawRoundedRect(D2D1::RoundedRect(rect, 6.0f, 6.0f), stroke, 1.0f);
        drawTextClipped(
            render,
            state.tabTitles[index],
            D2D1::RectF(rect.left + 12.0f, rect.top + 5.0f, rect.right - 32.0f, rect.bottom - 3.0f),
            rect,
            render.textFormat(),
            active ? textPrimary : textSecondary);

        const D2D1_RECT_F closeRect = tabCloseRect(rect);
        if (hasArea(closeRect)) {
            const D2D1_COLOR_F closeColor = active
                ? textSecondary
                : mutedText;
            render.drawLine(
                D2D1::Point2F(closeRect.left + 6.0f, closeRect.top + 5.0f),
                D2D1::Point2F(closeRect.right - 6.0f, closeRect.bottom - 5.0f),
                closeColor,
                1.2f);
            render.drawLine(
                D2D1::Point2F(closeRect.right - 6.0f, closeRect.top + 5.0f),
                D2D1::Point2F(closeRect.left + 6.0f, closeRect.bottom - 5.0f),
                closeColor,
                1.2f);
        }
    }

    const D2D1_RECT_F plusRect = newTabRect(visibleTabCount, rects.pathbar.right);
    if (isNewTabRectUsable(plusRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(plusRect, 6.0f, 6.0f),
            controlFill);
        render.drawRoundedRect(
            D2D1::RoundedRect(plusRect, 6.0f, 6.0f),
            controlStroke,
            1.0f);
        drawTextClipped(
            render,
            L"+",
            D2D1::RectF(plusRect.left + 10.0f, plusRect.top + 4.0f, plusRect.right, plusRect.bottom),
            plusRect,
            render.headerTextFormat(),
            textSecondary);
    }

    if (rects.sidebar.right < rects.pathbar.right) {
        drawSeparator(render, rects.sidebar.right, 0.0f, rects.sidebar.bottom, separatorColor);
    }
    render.drawLine(
        D2D1::Point2F(rects.toolbar.left, rects.toolbar.bottom),
        D2D1::Point2F(rects.toolbar.right, rects.toolbar.bottom),
        separatorColor);
    render.drawLine(
        D2D1::Point2F(rects.header.left, rects.header.bottom),
        D2D1::Point2F(rects.header.right, rects.header.bottom),
        separatorColor);
    render.drawLine(
        D2D1::Point2F(rects.pathbar.left, rects.pathbar.top),
        D2D1::Point2F(rects.pathbar.right, rects.pathbar.top),
        separatorColor);

    const std::vector<SidebarLayoutRow> sidebarRows = sidebarLayoutRows(state.sidebarItems);
    for (const SidebarLayoutRow& row : sidebarRows) {
        const SidebarItem& item = state.sidebarItems[row.index];
        const float rowY = row.rowY;

        if (row.header) {
            drawTextClipped(
                render,
                row.header,
                D2D1::RectF(18.0f, row.headerTop, 150.0f, row.headerTop + 21.0f),
                rects.sidebar,
                render.headerTextFormat(),
                mutedText);
        }

        if (item.selected) {
            render.fillRoundedRect(
                D2D1::RoundedRect(
                    clampRect(D2D1::RectF(kSidebarRowLeft, rowY - 2.0f, kSidebarRowRight, rowY + 24.0f), rects.sidebar),
                    6.0f,
                    6.0f),
                themeColor(mode, D2D1::ColorF(0.80f, 0.82f, 0.84f), D2D1::ColorF(0.13f, 0.22f, 0.36f)));
        }

        drawSidebarIcon(render, item.label, 35.0f, rowY + 4.0f, item.selected, item.available, mode);

        drawTextClipped(
            render,
            item.label,
            D2D1::RectF(kSidebarTextLeft, rowY + 2.0f, 160.0f, rowY + 22.0f),
            rects.sidebar,
            render.textFormat(),
            item.available ? textPrimary : mutedText);
    }

    render.fillRoundedRect(
        D2D1::RoundedRect(
            clampRect(
                D2D1::RectF(rects.toolbar.left + kBackLeftOffset, rects.toolbar.top + kToolbarButtonTop, rects.toolbar.left + kBackRightOffset, rects.toolbar.top + kToolbarButtonBottom),
                rects.toolbar),
            7.0f,
            7.0f),
        state.canGoBack ? controlFill : themeColor(mode, D2D1::ColorF(0.945f, 0.945f, 0.94f), D2D1::ColorF(0.095f, 0.115f, 0.160f)));
    render.fillRoundedRect(
        D2D1::RoundedRect(
            clampRect(
                D2D1::RectF(rects.toolbar.left + kForwardLeftOffset, rects.toolbar.top + kToolbarButtonTop, rects.toolbar.left + kForwardRightOffset, rects.toolbar.top + kToolbarButtonBottom),
                rects.toolbar),
            7.0f,
            7.0f),
        state.canGoForward ? controlFill : themeColor(mode, D2D1::ColorF(0.945f, 0.945f, 0.94f), D2D1::ColorF(0.095f, 0.115f, 0.160f)));
    drawTextClipped(
        render,
        L"\u2039",
        D2D1::RectF(rects.toolbar.left + 22.0f, rects.toolbar.top + 15.0f, rects.toolbar.left + 44.0f, rects.toolbar.top + 43.0f),
        rects.toolbar,
        render.textFormat(),
        navigationColor(state.canGoBack, mode));
    drawTextClipped(
        render,
        L"\u203a",
        D2D1::RectF(rects.toolbar.left + 56.0f, rects.toolbar.top + 15.0f, rects.toolbar.left + 82.0f, rects.toolbar.top + 43.0f),
        rects.toolbar,
        render.textFormat(),
        navigationColor(state.canGoForward, mode));
    drawTextClipped(
        render,
        title,
        D2D1::RectF(rects.toolbar.left + 106.0f, rects.toolbar.top + 16.0f, rects.toolbar.left + 240.0f, rects.toolbar.top + 45.0f),
        rects.toolbar,
        render.headerTextFormat(),
        textPrimary);
    const D2D1_RECT_F sortRect = sortButtonRect(rects.toolbar);
    if (hasArea(sortRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(sortRect, 7.0f, 7.0f),
            controlFill);
        render.drawRoundedRect(
            D2D1::RoundedRect(sortRect, 7.0f, 7.0f),
            controlStroke,
            1.0f);
        drawSortGlyph(render, sortRect, state.sortDirection, textSecondary);
    }

    const D2D1_RECT_F settingsRect = settingsButtonRect(rects.toolbar);
    if (hasArea(settingsRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(settingsRect, 7.0f, 7.0f),
            controlFill);
        render.drawRoundedRect(
            D2D1::RoundedRect(settingsRect, 7.0f, 7.0f),
            controlStroke,
            1.0f);
        drawSettingsGlyph(render, settingsRect, textSecondary);
    }

    const D2D1_RECT_F searchRect = searchFieldRect(rects.toolbar);
    if (searchRect.right - searchRect.left >= 80.0f) {
        const bool hasSearchText = !state.searchText.empty();
        const D2D1_RECT_F searchTextRect = D2D1::RectF(searchRect.left + 11.0f, searchRect.top + 4.0f, searchRect.right - 12.0f, searchRect.bottom);
        render.fillRoundedRect(
            D2D1::RoundedRect(searchRect, 7.0f, 7.0f),
            themeColor(mode, D2D1::ColorF(1.0f, 1.0f, 1.0f), D2D1::ColorF(0.095f, 0.115f, 0.160f)));
        render.drawRoundedRect(
            D2D1::RoundedRect(searchRect, 7.0f, 7.0f),
            state.searchFocused ? D2D1::ColorF(0.20f, 0.52f, 1.0f) : themeColor(mode, rgb(0.88f), D2D1::ColorF(0.22f, 0.27f, 0.36f)),
            state.searchFocused ? 1.4f : 1.0f);
        drawTextClipped(
            render,
            hasSearchText ? std::wstring_view(state.searchText) : (state.searchFocused ? std::wstring_view(L"") : std::wstring_view(L"\u2315 Search")),
            searchTextRect,
            searchRect,
            render.textFormat(),
            hasSearchText ? textPrimary : mutedText);
        if (state.searchFocused && state.searchCaretVisible) {
            const float caretX = (std::min)(searchTextRect.right, searchTextRect.left + approximateInlineTextWidth(state.searchText) + 1.0f);
            render.drawLine(
                D2D1::Point2F(caretX, searchRect.top + 7.0f),
                D2D1::Point2F(caretX, searchRect.bottom - 7.0f),
                textPrimary,
                1.2f);
        }
    }

    const HeaderColumnRects columns = headerColumnRects(rects.header, state);
    if (hasArea(columns.name)) {
        const D2D1_COLOR_F activeHeader = textPrimary;
        const D2D1_COLOR_F inactiveHeader = textSecondary;
        if (hasArea(columns.modified)) {
            drawHeaderColumnSeparator(render, rects.header, columns.modified.left, mode);
        }
        if (hasArea(columns.size)) {
            drawHeaderColumnSeparator(render, rects.header, columns.size.left, mode);
        }
        if (hasArea(columns.kind)) {
            drawHeaderColumnSeparator(render, rects.header, columns.kind.left, mode);
        }
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

    const D2D1_RECT_F pathRect = pathTextRect(rects);
    if (state.addressEditing) {
        const D2D1_RECT_F addressTextRect = D2D1::RectF(pathRect.left + 8.0f, pathRect.top, pathRect.right - 8.0f, pathRect.bottom);
        render.fillRoundedRect(
            D2D1::RoundedRect(pathRect, 6.0f, 6.0f),
            themeColor(mode, D2D1::ColorF(1.0f, 1.0f, 1.0f), D2D1::ColorF(0.095f, 0.115f, 0.160f)));
        render.drawRoundedRect(
            D2D1::RoundedRect(pathRect, 6.0f, 6.0f),
            D2D1::ColorF(0.20f, 0.52f, 1.0f),
            1.2f);
        const std::size_t selectionStart = (std::min)(state.addressSelectionStart, state.addressText.size());
        const std::size_t selectionEnd = (std::min)(state.addressSelectionEnd, state.addressText.size());
        if (selectionStart < selectionEnd) {
            const float selectionLeft = (std::min)(
                addressTextRect.right,
                addressTextRect.left + approximateInlineTextWidth(std::wstring_view(state.addressText).substr(0, selectionStart)));
            const float selectionRight = (std::min)(
                addressTextRect.right,
                addressTextRect.left + approximateInlineTextWidth(std::wstring_view(state.addressText).substr(0, selectionEnd)));
            if (selectionLeft < selectionRight) {
                render.fillRoundedRect(
                    D2D1::RoundedRect(D2D1::RectF(selectionLeft, pathRect.top + 4.0f, selectionRight, pathRect.bottom - 4.0f), 3.0f, 3.0f),
                    D2D1::ColorF(0.20f, 0.52f, 1.0f, 0.30f));
            }
        }
        drawTextClipped(
            render,
            state.addressText,
            addressTextRect,
            window,
            render.textFormat(),
            textPrimary);
        if (state.addressCaretVisible && selectionStart == selectionEnd) {
            const std::size_t caretIndex = (std::min)(state.addressCaretIndex, state.addressText.size());
            const float caretX = (std::min)(
                addressTextRect.right,
                addressTextRect.left + approximateInlineTextWidth(std::wstring_view(state.addressText).substr(0, caretIndex)) + 1.0f);
            render.drawLine(
                D2D1::Point2F(caretX, pathRect.top + 6.0f),
                D2D1::Point2F(caretX, pathRect.bottom - 6.0f),
                textPrimary,
                1.2f);
        }
    } else if (state.statusText.empty()) {
        drawPathSegments(render, clampRect(pathRect, window), state.pathText, mode);
    } else {
        drawTextClipped(
            render,
            state.statusText,
            pathRect,
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
        const D2D1_RECT_F closeRect = tabCloseRect(rect);
        if (hasArea(closeRect) && containsPoint(closeRect, x, y)) {
            return {ChromeHitKind::CloseTab, 0, index};
        }
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

    const HeaderColumnRects columns = headerColumnRects(rects.header, state);
    if (hasArea(columns.modified) && containsPoint(columnResizeRect(columns.modified.left, rects.header), x, y)) {
        return {ChromeHitKind::ResizeModifiedColumn, 0, 0};
    }
    if (hasArea(columns.size) && containsPoint(columnResizeRect(columns.size.left, rects.header), x, y)) {
        return {ChromeHitKind::ResizeSizeColumn, 0, 0};
    }
    if (hasArea(columns.kind) && containsPoint(columnResizeRect(columns.kind.left, rects.header), x, y)) {
        return {ChromeHitKind::ResizeKindColumn, 0, 0};
    }
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

    const std::vector<SidebarLayoutRow> sidebarRows = sidebarLayoutRows(state.sidebarItems);
    for (const SidebarLayoutRow& row : sidebarRows) {
        const SidebarItem& item = state.sidebarItems[row.index];
        const float rowY = row.rowY;
        const D2D1_RECT_F rowRect = D2D1::RectF(kSidebarRowLeft, rowY - 2.0f, kSidebarRowRight, rowY + 24.0f);
        if (item.available && containsPoint(rowRect, x, y)) {
            return {ChromeHitKind::SidebarItem, row.index, 0};
        }
    }

    if (state.addressEditing && containsPoint(rects.pathbar, x, y)) {
        return {ChromeHitKind::AddressField, 0, 0, {}};
    }

    if (!state.statusText.empty() && containsPoint(rects.pathbar, x, y)) {
        return {ChromeHitKind::AddressField, 0, 0, {}};
    }

    if (state.statusText.empty() && containsPoint(rects.pathbar, x, y)) {
        const D2D1_RECT_F pathRect = pathTextRect(rects);
        const std::vector<PathSegmentLayout> segments = pathSegmentLayouts(pathRect, state.pathText);
        for (const PathSegmentLayout& segment : segments) {
            if (!segment.targetPath.empty() && containsPoint(segment.rect, x, y)) {
                return {ChromeHitKind::PathSegment, 0, 0, segment.targetPath};
            }
        }
        if (!segments.empty() && x > segments.back().rect.right) {
            return {ChromeHitKind::AddressField, 0, 0, {}};
        }
    }

    return {};
}

}
