#include "ui/FinderChrome.h"
#include "ui/ThemeTokens.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace finderx {

namespace {

constexpr float kSidebarWidth = 174.0f;
constexpr float kTabStripHeight = 34.0f;
constexpr float kTabTop = 6.0f;
constexpr float kTabHeight = 28.0f;
constexpr float kTabLeftInset = 10.0f;
constexpr float kMinTabWidth = 92.0f;
constexpr float kPreferredTabWidth = 260.0f;
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
constexpr float kSidebarTextLeft = 56.0f;
constexpr float kBackLeftOffset = 18.0f;
constexpr float kBackRightOffset = 50.0f;
constexpr float kForwardLeftOffset = 54.0f;
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
constexpr float kTopBarGroupRadius = 7.0f;
constexpr float kTopBarButtonRadius = 6.0f;

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

struct ToolbarCommandLayout {
    ToolbarCommand command = ToolbarCommand::Search;
    D2D1_RECT_F rect{};
};

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

D2D1_RECT_F sidebarRowRect(const D2D1_RECT_F& sidebar, float rowY) {
    return clampRect(D2D1::RectF(kSidebarRowLeft, rowY - 2.0f, sidebar.right - 12.0f, rowY + 24.0f), sidebar);
}

D2D1_RECT_F sidebarTextRect(const D2D1_RECT_F& sidebar, float rowY) {
    return clampRect(D2D1::RectF(kSidebarTextLeft, rowY + 2.0f, sidebar.right - 18.0f, rowY + 22.0f), sidebar);
}

D2D1_RECT_F searchFieldRect(const D2D1_RECT_F& toolbar) {
    if (toolbar.right - toolbar.left < 240.0f) {
        return D2D1::RectF();
    }
    const float searchLeft = (std::max)(toolbar.left + 420.0f, toolbar.right - 202.0f);
    return clampRect(D2D1::RectF(searchLeft, toolbar.top + kToolbarButtonTop, toolbar.right - 14.0f, toolbar.top + kToolbarButtonBottom), toolbar);
}

bool hasToolbarCommand(const ChromeState& state, ToolbarCommand command) {
    return std::find(state.toolbarCommands.begin(), state.toolbarCommands.end(), command) != state.toolbarCommands.end();
}

ChromeHitKind hitKindForToolbarCommand(ToolbarCommand command) {
    switch (command) {
    case ToolbarCommand::NewFolder:
        return ChromeHitKind::NewFolder;
    case ToolbarCommand::NewFile:
        return ChromeHitKind::NewFile;
    case ToolbarCommand::Sort:
        return ChromeHitKind::SortMenu;
    case ToolbarCommand::Settings:
        return ChromeHitKind::Settings;
    case ToolbarCommand::PowerShell:
        return ChromeHitKind::OpenPowerShell;
    case ToolbarCommand::Search:
    default:
        return ChromeHitKind::SearchField;
    }
}

std::vector<ToolbarCommandLayout> toolbarCommandLayouts(const D2D1_RECT_F& toolbar, const ChromeState& state) {
    std::vector<ToolbarCommandLayout> layouts;
    std::vector<ToolbarCommand> commands;
    for (const ToolbarCommand command : state.toolbarCommands) {
        if (command != ToolbarCommand::Search) {
            commands.push_back(command);
        }
    }
    if (commands.empty()) {
        return layouts;
    }

    const bool hasSearch = hasToolbarCommand(state, ToolbarCommand::Search);
    const D2D1_RECT_F searchRect = hasSearch ? searchFieldRect(toolbar) : D2D1::RectF();
    const float maxRight = hasArea(searchRect) ? searchRect.left - kToolbarUtilitySearchGap : toolbar.right - 14.0f;
    const float minLeft = toolbar.left + 250.0f;
    const float totalWidth = static_cast<float>(commands.size()) * kToolbarUtilityButtonWidth
        + static_cast<float>(commands.size() - 1) * kToolbarUtilityButtonGap;
    float cursor = maxRight - totalWidth;
    if (cursor < minLeft) {
        cursor = minLeft;
    }

    for (const ToolbarCommand command : commands) {
        if (cursor + kToolbarUtilityButtonWidth > maxRight) {
            break;
        }
        layouts.push_back({command, D2D1::RectF(cursor, toolbar.top + kToolbarButtonTop, cursor + kToolbarUtilityButtonWidth, toolbar.top + kToolbarButtonBottom)});
        cursor += kToolbarUtilityButtonWidth + kToolbarUtilityButtonGap;
    }
    return layouts;
}

float toolbarTitleRight(const D2D1_RECT_F& toolbar, const ChromeState& state) {
    const std::vector<ToolbarCommandLayout> layouts = toolbarCommandLayouts(toolbar, state);
    if (!layouts.empty()) {
        return (std::max)(toolbar.left + 240.0f, layouts.front().rect.left - 14.0f);
    }
    const D2D1_RECT_F searchRect = hasToolbarCommand(state, ToolbarCommand::Search) ? searchFieldRect(toolbar) : D2D1::RectF();
    if (hasArea(searchRect)) {
        return (std::max)(toolbar.left + 240.0f, searchRect.left - 14.0f);
    }
    return toolbar.right - 14.0f;
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
    const ThemeTokens tokens = themeTokens(mode);
    render.drawLine(
        D2D1::Point2F(x, header.top + 5.0f),
        D2D1::Point2F(x, header.bottom - 5.0f),
        tokens.appLine,
        1.0f);
}

D2D1_RECT_F tabStripStart(float sidebarRight) {
    return D2D1::RectF(sidebarRight + kTabLeftInset, kTabTop, sidebarRight + kTabLeftInset, kTabTop + kTabHeight);
}

float tabWidth(std::size_t tabCount, float left, float right) {
    if (tabCount == 0 || right <= left) {
        return kMinTabWidth;
    }
    const float available = right - left - kNewTabWidth - kTabGap;
    const float gapWidth = static_cast<float>(tabCount > 0 ? tabCount - 1 : 0) * kTabGap;
    return (std::clamp)((available - gapWidth) / static_cast<float>(tabCount), kMinTabWidth, kPreferredTabWidth);
}

D2D1_RECT_F tabRect(std::size_t index, std::size_t tabCount, float sidebarRight, float right) {
    const float start = tabStripStart(sidebarRight).left;
    const float width = tabWidth(tabCount, start, right);
    const float left = start + (static_cast<float>(index) * (width + kTabGap));
    if (left >= right) {
        return D2D1::RectF(right, kTabTop, right, kTabTop + kTabHeight);
    }

    return D2D1::RectF(left, kTabTop, (std::min)(left + width, right), kTabTop + kTabHeight);
}

D2D1_RECT_F tabCloseRect(const D2D1_RECT_F& tab) {
    if (tab.right - tab.left <= 72.0f) {
        return D2D1::RectF();
    }

    const float size = 22.0f;
    const float left = tab.right - 30.0f;
    const float top = tab.top + (tab.bottom - tab.top - size) * 0.5f;
    return D2D1::RectF(left, top, left + size, top + size);
}

D2D1_RECT_F newTabRect(std::size_t tabCount, float sidebarRight, float right) {
    const float start = tabStripStart(sidebarRight).left;
    const float width = tabWidth(tabCount, start, right);
    const float left = start + (static_cast<float>(tabCount) * (width + kTabGap));
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

void drawTextCenteredClipped(
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

    const float available = rect.right - rect.left;
    const float measured = render.measureTextWidth(text, format);
    if (measured > 0.0f && measured < available) {
        const float left = rect.left + (available - measured) * 0.5f;
        rect = D2D1::RectF(left, rect.top, left + measured + 2.0f, rect.bottom);
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

void drawSpaceTopBarButton(
    RenderContext& render,
    const D2D1_RECT_F& rect,
    D2D1_COLOR_F fill,
    D2D1_COLOR_F stroke,
    bool enabled = true,
    bool borderless = false) {
    const D2D1_COLOR_F buttonFill = enabled ? fill : withAlpha(fill, 0.52f);
    const D2D1_COLOR_F buttonStroke = enabled ? stroke : withAlpha(stroke, 0.48f);
    render.fillRoundedRect(
        D2D1::RoundedRect(rect, kTopBarButtonRadius, kTopBarButtonRadius),
        buttonFill);
    if (borderless) {
        return;
    }
    render.drawRoundedRect(
        D2D1::RoundedRect(rect, kTopBarButtonRadius, kTopBarButtonRadius),
        buttonStroke,
        1.0f);
}

void drawSpaceTopBarGroup(
    RenderContext& render,
    const D2D1_RECT_F& rect,
    D2D1_COLOR_F fill,
    D2D1_COLOR_F stroke) {
    render.fillRoundedRect(
        D2D1::RoundedRect(rect, kTopBarGroupRadius, kTopBarGroupRadius),
        fill);
    render.drawRoundedRect(
        D2D1::RoundedRect(rect, kTopBarGroupRadius, kTopBarGroupRadius),
        stroke,
        1.0f);
}

std::wstring sortedHeaderLabel(std::wstring label, SortColumn column, const ChromeState& state) {
    if (state.sortColumn != column) {
        return label;
    }

    label += state.sortDirection == SortDirection::Ascending ? L" \u25B2" : L" \u25BC";
    return label;
}

D2D1_COLOR_F navigationColor(bool enabled, ThemeMode mode) {
    const ThemeTokens tokens = themeTokens(mode);
    return enabled ? tokens.navigation : tokens.navigationDisabled;
}

D2D1_RECT_F centeredIconBox(const D2D1_RECT_F& rect, float size) {
    const float left = (rect.left + rect.right - size) * 0.5f;
    const float top = (rect.top + rect.bottom - size) * 0.5f;
    return D2D1::RectF(left, top, left + size, top + size);
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
    const ThemeTokens tokens = themeTokens(mode);
    const D2D1_COLOR_F fill = !available
        ? tokens.sidebarIconPlateDisabled
        : (selected ? tokens.sidebarIconPlateSelected : tokens.sidebarIconPlate);
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(x - 5.0f, y - 3.0f, x + 21.0f, y + 21.0f), 6.0f, 6.0f),
        fill);
}

void drawSidebarIcon(RenderContext& render, std::wstring_view label, float x, float y, bool selected, bool available, ThemeMode mode) {
    drawSidebarIconPlate(render, x, y, selected, available, mode);

    const ThemeTokens tokens = themeTokens(mode);
    const D2D1_COLOR_F color = !available
        ? tokens.sidebarIconDisabled
        : (selected ? tokens.sidebarIconSelected : tokens.sidebarIcon);

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
    (void)direction;
    const D2D1_RECT_F box = centeredIconBox(rect, 18.0f);
    const float leftX = box.left + 5.0f;
    const float rightX = box.right - 5.0f;
    render.drawLine(D2D1::Point2F(leftX, box.top + 3.0f), D2D1::Point2F(leftX, box.bottom - 3.0f), color, 1.35f);
    render.drawLine(D2D1::Point2F(leftX, box.bottom - 3.0f), D2D1::Point2F(leftX - 3.0f, box.bottom - 6.0f), color, 1.35f);
    render.drawLine(D2D1::Point2F(leftX, box.bottom - 3.0f), D2D1::Point2F(leftX + 3.0f, box.bottom - 6.0f), color, 1.35f);
    render.drawLine(D2D1::Point2F(rightX, box.bottom - 3.0f), D2D1::Point2F(rightX, box.top + 3.0f), color, 1.35f);
    render.drawLine(D2D1::Point2F(rightX, box.top + 3.0f), D2D1::Point2F(rightX - 3.0f, box.top + 6.0f), color, 1.35f);
    render.drawLine(D2D1::Point2F(rightX, box.top + 3.0f), D2D1::Point2F(rightX + 3.0f, box.top + 6.0f), color, 1.35f);
}

void drawSettingsGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const D2D1_RECT_F box = centeredIconBox(rect, 18.0f);
    const float cx = (box.left + box.right) * 0.5f;
    const float cy = (box.top + box.bottom) * 0.5f;
    render.drawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 4.2f, 4.2f), color, 1.3f);
    render.drawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), 7.4f, 7.4f), color, 1.15f);
    for (int index = 0; index < 8; ++index) {
        const float angle = static_cast<float>(index) * 0.7853982f;
        const float dx = std::cos(angle);
        const float dy = std::sin(angle);
        render.drawLine(
            D2D1::Point2F(cx + dx * 7.6f, cy + dy * 7.6f),
            D2D1::Point2F(cx + dx * 9.0f, cy + dy * 9.0f),
            color,
            1.25f);
    }
}

void drawNewFolderGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const D2D1_RECT_F icon = centeredIconBox(rect, 18.0f);
    const D2D1_RECT_F folder = D2D1::RectF(icon.left + 1.0f, icon.top + 3.0f, icon.right - 1.0f, icon.bottom - 1.5f);
    render.drawLine(D2D1::Point2F(folder.left + 1.0f, folder.top + 3.5f), D2D1::Point2F(folder.left + 4.4f, folder.top), color, 1.4f);
    render.drawLine(D2D1::Point2F(folder.left + 4.4f, folder.top), D2D1::Point2F(folder.left + 8.8f, folder.top), color, 1.4f);
    render.drawLine(D2D1::Point2F(folder.left + 8.8f, folder.top), D2D1::Point2F(folder.left + 11.0f, folder.top + 3.5f), color, 1.4f);
    render.drawRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(folder.left + 1.0f, folder.top + 3.5f, folder.right, folder.bottom), 3.0f, 3.0f),
        color,
        1.4f);
    const float plusX = folder.right - 4.5f;
    const float plusY = folder.bottom - 4.5f;
    render.drawLine(D2D1::Point2F(plusX - 3.0f, plusY), D2D1::Point2F(plusX + 3.0f, plusY), color, 1.4f);
    render.drawLine(D2D1::Point2F(plusX, plusY - 3.0f), D2D1::Point2F(plusX, plusY + 3.0f), color, 1.4f);
}

void drawNewFileGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const D2D1_RECT_F icon = centeredIconBox(rect, 18.0f);
    const D2D1_RECT_F page = D2D1::RectF(icon.left + 3.0f, icon.top + 1.5f, icon.right - 2.0f, icon.bottom - 1.5f);
    render.drawLine(D2D1::Point2F(page.left, page.top + 1.5f), D2D1::Point2F(page.left, page.bottom), color, 1.35f);
    render.drawLine(D2D1::Point2F(page.left, page.bottom), D2D1::Point2F(page.right, page.bottom), color, 1.35f);
    render.drawLine(D2D1::Point2F(page.right, page.top + 5.0f), D2D1::Point2F(page.right, page.bottom), color, 1.35f);
    render.drawLine(D2D1::Point2F(page.left + 1.5f, page.top), D2D1::Point2F(page.right - 5.0f, page.top), color, 1.35f);
    render.drawLine(D2D1::Point2F(page.right - 5.0f, page.top), D2D1::Point2F(page.right, page.top + 5.0f), color, 1.35f);
    render.drawLine(D2D1::Point2F(page.right - 5.0f, page.top), D2D1::Point2F(page.right - 5.0f, page.top + 5.0f), color, 1.15f);
    render.drawLine(D2D1::Point2F(page.right - 5.0f, page.top + 5.0f), D2D1::Point2F(page.right, page.top + 5.0f), color, 1.15f);
    const float plusX = page.right - 4.5f;
    const float plusY = page.bottom - 4.5f;
    render.drawLine(D2D1::Point2F(plusX - 3.0f, plusY), D2D1::Point2F(plusX + 3.0f, plusY), color, 1.35f);
    render.drawLine(D2D1::Point2F(plusX, plusY - 3.0f), D2D1::Point2F(plusX, plusY + 3.0f), color, 1.35f);
}

void drawPlusGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const D2D1_RECT_F box = centeredIconBox(rect, 13.0f);
    const float cx = (box.left + box.right) * 0.5f;
    const float cy = (box.top + box.bottom) * 0.5f;
    render.drawLine(D2D1::Point2F(box.left + 2.0f, cy), D2D1::Point2F(box.right - 2.0f, cy), color, 1.65f);
    render.drawLine(D2D1::Point2F(cx, box.top + 2.0f), D2D1::Point2F(cx, box.bottom - 2.0f), color, 1.65f);
}

void drawChevronGlyph(RenderContext& render, const D2D1_RECT_F& rect, bool left, D2D1_COLOR_F color) {
    const D2D1_RECT_F box = centeredIconBox(rect, 16.0f);
    const float midY = (box.top + box.bottom) * 0.5f;
    if (left) {
        render.drawLine(D2D1::Point2F(box.right - 5.0f, box.top + 3.5f), D2D1::Point2F(box.left + 5.0f, midY), color, 1.7f);
        render.drawLine(D2D1::Point2F(box.left + 5.0f, midY), D2D1::Point2F(box.right - 5.0f, box.bottom - 3.5f), color, 1.7f);
    } else {
        render.drawLine(D2D1::Point2F(box.left + 5.0f, box.top + 3.5f), D2D1::Point2F(box.right - 5.0f, midY), color, 1.7f);
        render.drawLine(D2D1::Point2F(box.right - 5.0f, midY), D2D1::Point2F(box.left + 5.0f, box.bottom - 3.5f), color, 1.7f);
    }
}

void drawToolbarCommandGlyph(RenderContext& render, ToolbarCommand command, const D2D1_RECT_F& rect, SortDirection sortDirection, D2D1_COLOR_F color) {
    switch (command) {
    case ToolbarCommand::NewFolder:
        drawNewFolderGlyph(render, rect, color);
        break;
    case ToolbarCommand::NewFile:
        drawNewFileGlyph(render, rect, color);
        break;
    case ToolbarCommand::Sort:
        drawSortGlyph(render, rect, sortDirection, color);
        break;
    case ToolbarCommand::Settings:
        drawSettingsGlyph(render, rect, color);
        break;
    case ToolbarCommand::PowerShell: {
        const D2D1_RECT_F shell = D2D1::RectF(rect.left + 7.0f, rect.top + 8.0f, rect.right - 7.0f, rect.bottom - 8.0f);
        render.drawRoundedRect(D2D1::RoundedRect(shell, 3.0f, 3.0f), color, 1.35f);
        render.drawLine(D2D1::Point2F(shell.left + 4.0f, shell.top + 5.0f), D2D1::Point2F(shell.left + 8.0f, shell.top + 9.0f), color, 1.35f);
        render.drawLine(D2D1::Point2F(shell.left + 8.0f, shell.top + 9.0f), D2D1::Point2F(shell.left + 4.0f, shell.top + 13.0f), color, 1.35f);
        render.drawLine(D2D1::Point2F(shell.left + 11.0f, shell.top + 13.0f), D2D1::Point2F(shell.right - 4.0f, shell.top + 13.0f), color, 1.35f);
        break;
    }
    case ToolbarCommand::Search:
        break;
    }
}

void drawSearchGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const D2D1_POINT_2F center = D2D1::Point2F(rect.left + 16.0f, rect.top + 12.5f);
    render.drawEllipse(D2D1::Ellipse(center, 5.2f, 5.2f), color, 1.35f);
    render.drawLine(D2D1::Point2F(center.x + 3.8f, center.y + 3.8f), D2D1::Point2F(center.x + 8.0f, center.y + 8.0f), color, 1.35f);
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

float measuredInlineTextWidth(RenderContext& render, std::wstring_view text) {
    const float measured = render.measureTextWidth(text, render.textFormat());
    return measured > 0.0f || text.empty() ? measured : approximateInlineTextWidth(text);
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
    const ThemeTokens tokens = themeTokens(mode);
    const D2D1_COLOR_F textColor = tokens.pathInk;
    const D2D1_COLOR_F chevronColor = tokens.pathChevron;

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
    return layout(width, height, kSidebarWidth);
}

LayoutRects FinderChrome::layout(float width, float height, float sidebarWidth) const {
    const float right = clampNonNegative(width);
    const float bottom = clampNonNegative(height);
    const float contentLeft = (std::min)(clampNonNegative(sidebarWidth), right);
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
    const ThemeTokens tokens = themeTokens(mode);
    const D2D1_COLOR_F separatorColor = tokens.appLine;
    const D2D1_COLOR_F textPrimary = tokens.ink;
    const D2D1_COLOR_F textSecondary = tokens.inkDull;
    const D2D1_COLOR_F mutedText = tokens.inkFaint;
    const D2D1_COLOR_F controlStroke = tokens.appLine;
    const D2D1_COLOR_F topBarSurface = withAlpha(tokens.appBox, isDarkTheme(mode) ? 0.66f : 0.82f);
    const D2D1_COLOR_F topBarButton = withAlpha(tokens.appOverlay, isDarkTheme(mode) ? 0.78f : 0.92f);
    const D2D1_COLOR_F topBarButtonMuted = withAlpha(tokens.appOverlay, isDarkTheme(mode) ? 0.42f : 0.54f);

    render.fillRect(rects.sidebar, tokens.sidebar);
    render.fillRect(rects.toolbar, tokens.menu);
    render.fillRect(rects.header, tokens.app);
    render.fillRect(rects.pathbar, tokens.menu);
    render.fillRect(
        D2D1::RectF(0.0f, 0.0f, rects.pathbar.right, (std::min)(kTabStripHeight, rects.sidebar.bottom)),
        tokens.sidebar);
    if (rects.sidebar.right >= 120.0f && rects.sidebar.bottom >= 62.0f) {
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(18.0f, 22.0f, 42.0f, 46.0f), tokens.radiusPanel, tokens.radiusPanel), tokens.accentDeep);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(24.0f, 28.0f, 36.0f, 40.0f), 3.0f, 3.0f), withAlpha(tokens.inkOnAccent, 0.22f));
        drawTextClipped(
            render,
            L"FinderX",
            D2D1::RectF(52.0f, 23.0f, 158.0f, 46.0f),
            rects.sidebar,
            render.headerTextFormat(),
            textPrimary);
    }
    std::size_t visibleTabCount = 0;
    for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
        const D2D1_RECT_F rect = tabRect(index, state.tabTitles.size(), rects.sidebar.right, rects.pathbar.right);
        if (!isTabRectUsable(rect)) {
            break;
        }
        visibleTabCount = index + 1;

        const bool active = index == state.activeTabIndex;
        const D2D1_COLOR_F fill = active
            ? (isDarkTheme(mode) ? withAlpha(tokens.appHover, 0.98f) : tokens.appBox)
            : withAlpha(tokens.appOverlay, isDarkTheme(mode) ? 0.52f : 0.64f);

        render.fillRoundedRect(D2D1::RoundedRect(rect, 0.0f, 0.0f), fill);
        drawTextCenteredClipped(
            render,
            state.tabTitles[index],
            D2D1::RectF(rect.left + 12.0f, rect.top + 5.0f, rect.right - 32.0f, rect.bottom - 3.0f),
            rect,
            render.textFormat(),
            active ? textPrimary : textSecondary);

        const D2D1_RECT_F closeRect = tabCloseRect(rect);
        if (hasArea(closeRect)) {
            const bool closeHovered = state.hasHoveredCloseTab && state.hoveredCloseTabIndex == index;
            const D2D1_COLOR_F closeColor = active
                ? textSecondary
                : mutedText;
            if (closeHovered) {
                const D2D1_COLOR_F closeHoverFill = active
                    ? (isDarkTheme(mode) ? tokens.appActive : tokens.appSelected)
                    : withAlpha(tokens.appHover, isDarkTheme(mode) ? 0.90f : 0.78f);
                render.fillRoundedRect(
                    D2D1::RoundedRect(closeRect, 4.0f, 4.0f),
                    closeHoverFill);
            }
            render.drawLine(
                D2D1::Point2F(closeRect.left + 6.0f, closeRect.top + 6.0f),
                D2D1::Point2F(closeRect.right - 6.0f, closeRect.bottom - 6.0f),
                closeColor,
                1.4f);
            render.drawLine(
                D2D1::Point2F(closeRect.right - 6.0f, closeRect.top + 6.0f),
                D2D1::Point2F(closeRect.left + 6.0f, closeRect.bottom - 6.0f),
                closeColor,
                1.4f);
        }
    }

    const D2D1_RECT_F plusRect = newTabRect(visibleTabCount, rects.sidebar.right, rects.pathbar.right);
    if (isNewTabRectUsable(plusRect)) {
        render.fillRoundedRect(
            D2D1::RoundedRect(plusRect, 15.0f, 15.0f),
            tokens.appBox);
        drawPlusGlyph(render, plusRect, textSecondary);
    }

    if (rects.sidebar.right < rects.pathbar.right) {
        drawSeparator(render, rects.sidebar.right, 0.0f, rects.sidebar.bottom, separatorColor);
    }
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
                    clampRect(D2D1::RectF(14.0f, rowY + 3.0f, 18.0f, rowY + 19.0f), rects.sidebar),
                    2.0f,
                    2.0f),
                tokens.accent);
            render.fillRoundedRect(
                D2D1::RoundedRect(
                    sidebarRowRect(rects.sidebar, rowY),
                    tokens.radiusPanel,
                    tokens.radiusPanel),
                withAlpha(tokens.sidebarSelected, 0.92f));
        }

        drawSidebarIcon(render, item.label, 35.0f, rowY + 4.0f, item.selected, item.available, mode);

        drawTextClipped(
            render,
            item.label,
            sidebarTextRect(rects.sidebar, rowY),
            rects.sidebar,
            render.textFormat(),
            item.available ? textPrimary : mutedText);
    }

    const D2D1_RECT_F backRect = clampRect(
        D2D1::RectF(rects.toolbar.left + kBackLeftOffset, rects.toolbar.top + kToolbarButtonTop, rects.toolbar.left + kBackRightOffset, rects.toolbar.top + kToolbarButtonBottom),
        rects.toolbar);
    const D2D1_RECT_F forwardRect = clampRect(
        D2D1::RectF(rects.toolbar.left + kForwardLeftOffset, rects.toolbar.top + kToolbarButtonTop, rects.toolbar.left + kForwardRightOffset, rects.toolbar.top + kToolbarButtonBottom),
        rects.toolbar);
    const D2D1_RECT_F navGroupRect = clampRect(
        D2D1::RectF(backRect.left - 1.0f, backRect.top - 1.0f, forwardRect.right + 1.0f, forwardRect.bottom + 1.0f),
        rects.toolbar);
    drawSpaceTopBarGroup(render, navGroupRect, topBarSurface, withAlpha(controlStroke, 0.78f));
    drawSpaceTopBarButton(render, backRect, state.canGoBack ? topBarButton : topBarButtonMuted, withAlpha(controlStroke, 0.50f), state.canGoBack);
    drawSpaceTopBarButton(render, forwardRect, state.canGoForward ? topBarButton : topBarButtonMuted, withAlpha(controlStroke, 0.50f), state.canGoForward);
    drawSeparator(
        render,
        (backRect.right + forwardRect.left) * 0.5f,
        navGroupRect.top + 6.0f,
        navGroupRect.bottom - 6.0f,
        withAlpha(controlStroke, 0.65f));
    drawChevronGlyph(
        render,
        backRect,
        true,
        navigationColor(state.canGoBack, mode));
    drawChevronGlyph(
        render,
        forwardRect,
        false,
        navigationColor(state.canGoForward, mode));
    drawTextClipped(
        render,
        title,
        D2D1::RectF(rects.toolbar.left + 106.0f, rects.toolbar.top + 16.0f, toolbarTitleRight(rects.toolbar, state), rects.toolbar.top + 45.0f),
        rects.toolbar,
        render.headerTextFormat(),
        textPrimary);
    const std::vector<ToolbarCommandLayout> toolbarLayouts = toolbarCommandLayouts(rects.toolbar, state);
    for (const ToolbarCommandLayout& layout : toolbarLayouts) {
        const bool hovered = state.hasHoveredToolbarCommand && state.hoveredToolbarCommand == layout.command;
        if (hovered) {
            drawSpaceTopBarButton(render, layout.rect, topBarButton, withAlpha(controlStroke, 0.0f), true, true);
        }
        drawToolbarCommandGlyph(render, layout.command, layout.rect, state.sortDirection, textSecondary);
    }

    const D2D1_RECT_F searchRect = hasToolbarCommand(state, ToolbarCommand::Search) ? searchFieldRect(rects.toolbar) : D2D1::RectF();
    if (hasArea(searchRect) && searchRect.right - searchRect.left >= 80.0f) {
        const bool hasSearchText = !state.searchText.empty();
        const D2D1_RECT_F searchTextRect = D2D1::RectF(searchRect.left + 34.0f, searchRect.top + 4.0f, searchRect.right - 12.0f, searchRect.bottom);
        render.fillRoundedRect(
            D2D1::RoundedRect(searchRect, 8.0f, 8.0f),
            withAlpha(tokens.appInput, isDarkTheme(mode) ? 0.88f : 0.96f));
        render.drawRoundedRect(
            D2D1::RoundedRect(searchRect, 8.0f, 8.0f),
            state.searchFocused ? withAlpha(tokens.accent, 0.92f) : withAlpha(tokens.appLine, 0.78f),
            state.searchFocused ? 1.35f : 1.0f);
        drawSearchGlyph(render, searchRect, state.searchFocused ? tokens.accent : mutedText);
        drawTextClipped(
            render,
            hasSearchText ? std::wstring_view(state.searchText) : (state.searchFocused ? std::wstring_view(L"") : std::wstring_view(L"Search")),
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
        auto paddedColumn = [](D2D1_RECT_F rect) {
            rect.left += 10.0f;
            return rect;
        };
        drawTextClipped(render, sortedHeaderLabel(L"Name", SortColumn::Name, state), columns.name, rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Name ? activeHeader : inactiveHeader);
        if (hasArea(columns.modified)) {
            drawTextClipped(render, sortedHeaderLabel(L"Date Modified", SortColumn::Modified, state), paddedColumn(columns.modified), rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Modified ? activeHeader : inactiveHeader);
        }
        if (hasArea(columns.size)) {
            drawTextClipped(render, sortedHeaderLabel(L"Size", SortColumn::Size, state), paddedColumn(columns.size), rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Size ? activeHeader : inactiveHeader);
        }
        if (hasArea(columns.kind)) {
            drawTextClipped(render, sortedHeaderLabel(L"Kind", SortColumn::Kind, state), paddedColumn(columns.kind), rects.header, render.headerTextFormat(), state.sortColumn == SortColumn::Kind ? activeHeader : inactiveHeader);
        }
    }

    const D2D1_RECT_F pathRect = pathTextRect(rects);
    if (state.addressEditing) {
        const D2D1_RECT_F addressTextRect = D2D1::RectF(pathRect.left + 8.0f, pathRect.top, pathRect.right - 8.0f, pathRect.bottom);
        render.fillRoundedRect(
            D2D1::RoundedRect(pathRect, tokens.radiusControl, tokens.radiusControl),
            tokens.appInput);
        render.drawRoundedRect(
            D2D1::RoundedRect(pathRect, tokens.radiusControl, tokens.radiusControl),
            tokens.accent,
            1.2f);
        const std::size_t selectionStart = (std::min)(state.addressSelectionStart, state.addressText.size());
        const std::size_t selectionEnd = (std::min)(state.addressSelectionEnd, state.addressText.size());
        if (selectionStart < selectionEnd) {
            const float selectionLeft = (std::min)(
                addressTextRect.right,
                addressTextRect.left + measuredInlineTextWidth(render, std::wstring_view(state.addressText).substr(0, selectionStart)));
            const float selectionRight = (std::min)(
                addressTextRect.right,
                addressTextRect.left + measuredInlineTextWidth(render, std::wstring_view(state.addressText).substr(0, selectionEnd)));
            if (selectionLeft < selectionRight) {
                render.fillRoundedRect(
                    D2D1::RoundedRect(D2D1::RectF(selectionLeft, pathRect.top + 4.0f, selectionRight, pathRect.bottom - 4.0f), 3.0f, 3.0f),
                    withAlpha(tokens.accent, 0.30f));
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
                addressTextRect.left + measuredInlineTextWidth(render, std::wstring_view(state.addressText).substr(0, caretIndex)) + 1.0f);
            render.drawLine(
                D2D1::Point2F(caretX, pathRect.top + 6.0f),
                D2D1::Point2F(caretX, pathRect.bottom - 6.0f),
                textPrimary,
                1.2f);
        }
    } else if (state.statusText.empty()) {
        const D2D1_RECT_F clippedPathRect = clampRect(pathRect, window);
        drawPathSegments(render, clippedPathRect, state.pathText, mode);
    } else {
        drawTextClipped(
            render,
            state.statusText,
            pathRect,
            window,
            render.textFormat(),
            tokens.statusError);
    }
}

ChromeHitResult FinderChrome::hitTest(float x, float y, const LayoutRects& rects, const ChromeState& state) const {
    if (rects.sidebar.right > 0.0f && y >= kTabStripHeight && y <= rects.sidebar.bottom && std::abs(x - rects.sidebar.right) <= kColumnResizeHitSlop) {
        return {ChromeHitKind::ResizeSidebar, 0, 0};
    }

    std::size_t visibleTabCount = 0;
    for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
        const D2D1_RECT_F rect = tabRect(index, state.tabTitles.size(), rects.sidebar.right, rects.pathbar.right);
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

    const D2D1_RECT_F plusRect = newTabRect(visibleTabCount, rects.sidebar.right, rects.pathbar.right);
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

    const std::vector<ToolbarCommandLayout> toolbarLayouts = toolbarCommandLayouts(rects.toolbar, state);
    for (const ToolbarCommandLayout& layout : toolbarLayouts) {
        if (containsPoint(layout.rect, x, y)) {
            return {hitKindForToolbarCommand(layout.command), 0, 0};
        }
    }

    const D2D1_RECT_F searchRect = hasToolbarCommand(state, ToolbarCommand::Search) ? searchFieldRect(rects.toolbar) : D2D1::RectF();
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
        const D2D1_RECT_F rowRect = sidebarRowRect(rects.sidebar, rowY);
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
