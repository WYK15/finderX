#include "ui/FinderChrome.h"
#include "ui/ThemeTokens.h"

#include <algorithm>
#include <array>
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

    return D2D1::RectF(tab.right - 28.0f, tab.top + 6.0f, tab.right - 8.0f, tab.bottom - 6.0f);
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

constexpr std::string_view kSvgCaretLeft = "M165.66,202.34a8,8,0,0,1-11.32,11.32l-80-80a8,8,0,0,1,0-11.32l80-80a8,8,0,0,1,11.32,11.32L91.31,128Z";
constexpr std::string_view kSvgCaretRight = "M181.66,133.66l-80,80a8,8,0,0,1-11.32-11.32L164.69,128,90.34,53.66a8,8,0,0,1,11.32-11.32l80,80A8,8,0,0,1,181.66,133.66Z";
constexpr std::string_view kSvgPlus = "M224,128a8,8,0,0,1-8,8H136v80a8,8,0,0,1-16,0V136H40a8,8,0,0,1,0-16h80V40a8,8,0,0,1,16,0v80h80A8,8,0,0,1,224,128Z";
constexpr std::string_view kSvgFolderPlus = "M216,72H131.31L104,44.69A15.86,15.86,0,0,0,92.69,40H40A16,16,0,0,0,24,56V200.62A15.4,15.4,0,0,0,39.38,216H216.89A15.13,15.13,0,0,0,232,200.89V88A16,16,0,0,0,216,72ZM92.69,56l16,16H40V56ZM216,200H40V88H216Zm-88-88a8,8,0,0,1,8,8v16h16a8,8,0,0,1,0,16H136v16a8,8,0,0,1-16,0V152H104a8,8,0,0,1,0-16h16V120A8,8,0,0,1,128,112Z";
constexpr std::string_view kSvgFilePlus = "M213.66,82.34l-56-56A8,8,0,0,0,152,24H56A16,16,0,0,0,40,40V216a16,16,0,0,0,16,16H200a16,16,0,0,0,16-16V88A8,8,0,0,0,213.66,82.34ZM160,51.31,188.69,80H160ZM200,216H56V40h88V88a8,8,0,0,0,8,8h48V216Zm-40-64a8,8,0,0,1-8,8H136v16a8,8,0,0,1-16,0V160H104a8,8,0,0,1,0-16h16V128a8,8,0,0,1,16,0v16h16A8,8,0,0,1,160,152Z";
constexpr std::string_view kSvgArrowsDownUp = "M117.66,170.34a8,8,0,0,1,0,11.32l-32,32a8,8,0,0,1-11.32,0l-32-32a8,8,0,0,1,11.32-11.32L72,188.69V48a8,8,0,0,1,16,0V188.69l18.34-18.35A8,8,0,0,1,117.66,170.34Zm96-96-32-32a8,8,0,0,0-11.32,0l-32,32a8,8,0,0,0,11.32,11.32L168,67.31V208a8,8,0,0,0,16,0V67.31l18.34,18.35a8,8,0,0,0,11.32-11.32Z";
constexpr std::string_view kSvgGearSix = "M128,80a48,48,0,1,0,48,48A48.05,48.05,0,0,0,128,80Zm0,80a32,32,0,1,1,32-32A32,32,0,0,1,128,160Zm109.94-52.79a8,8,0,0,0-3.89-5.4l-29.83-17-.12-33.62a8,8,0,0,0-2.83-6.08,111.91,111.91,0,0,0-36.72-20.67,8,8,0,0,0-6.46.59L128,41.85,97.88,25a8,8,0,0,0-6.47-.6A112.1,112.1,0,0,0,54.73,45.15a8,8,0,0,0-2.83,6.07l-.15,33.65-29.83,17a8,8,0,0,0-3.89,5.4,106.47,106.47,0,0,0,0,41.56,8,8,0,0,0,3.89,5.4l29.83,17,.12,33.62a8,8,0,0,0,2.83,6.08,111.91,111.91,0,0,0,36.72,20.67,8,8,0,0,0,6.46-.59L128,214.15,158.12,231a7.91,7.91,0,0,0,3.9,1,8.09,8.09,0,0,0,2.57-.42,112.1,112.1,0,0,0,36.68-20.73,8,8,0,0,0,2.83-6.07l.15-33.65,29.83-17a8,8,0,0,0,3.89-5.4A106.47,106.47,0,0,0,237.94,107.21Zm-15,34.91-28.57,16.25a8,8,0,0,0-3,3c-.58,1-1.19,2.06-1.81,3.06a7.94,7.94,0,0,0-1.22,4.21l-.15,32.25a95.89,95.89,0,0,1-25.37,14.3L134,199.13a8,8,0,0,0-3.91-1h-.19c-1.21,0-2.43,0-3.64,0a8.08,8.08,0,0,0-4.1,1l-28.84,16.1A96,96,0,0,1,67.88,201l-.11-32.2a8,8,0,0,0-1.22-4.22c-.62-1-1.23-2-1.8-3.06a8.09,8.09,0,0,0-3-3.06l-28.6-16.29a90.49,90.49,0,0,1,0-28.26L61.67,97.63a8,8,0,0,0,3-3c.58-1,1.19-2.06,1.81-3.06a7.94,7.94,0,0,0,1.22-4.21l.15-32.25a95.89,95.89,0,0,1,25.37-14.3L122,56.87a8,8,0,0,0,4.1,1c1.21,0,2.43,0,3.64,0a8.08,8.08,0,0,0,4.1-1l28.84-16.1A96,96,0,0,1,188.12,55l.11,32.2a8,8,0,0,0,1.22,4.22c.62,1,1.23,2,1.8,3.06a8.09,8.09,0,0,0,3,3.06l28.6,16.29A90.49,90.49,0,0,1,222.9,142.12Z";
constexpr std::string_view kSvgMagnifyingGlass = "M229.66,218.34l-50.07-50.06a88.11,88.11,0,1,0-11.31,11.31l50.06,50.07a8,8,0,0,0,11.32-11.32ZM40,112a72,72,0,1,1,72,72A72.08,72.08,0,0,1,40,112Z";

D2D1_RECT_F centeredSvgRect(const D2D1_RECT_F& rect, float size) {
    const float left = (rect.left + rect.right - size) * 0.5f;
    const float top = (rect.top + rect.bottom - size) * 0.5f;
    return D2D1::RectF(left, top, left + size, top + size);
}

void drawSvgIcon(RenderContext& render, std::string_view pathData, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float size = 18.0f) {
    render.fillSvgPath(pathData, 256.0f, 256.0f, centeredSvgRect(rect, size), color);
}

D2D1_RECT_F centeredIconBox(const D2D1_RECT_F& rect, float size) {
    const float left = (rect.left + rect.right - size) * 0.5f;
    const float top = (rect.top + rect.bottom - size) * 0.5f;
    return D2D1::RectF(left, top, left + size, top + size);
}

D2D1_POINT_2F phosphorPoint(const D2D1_RECT_F& box, float x, float y) {
    const float width = box.right - box.left;
    const float height = box.bottom - box.top;
    return D2D1::Point2F(box.left + (x / 256.0f) * width, box.top + (y / 256.0f) * height);
}

D2D1_RECT_F phosphorRect(const D2D1_RECT_F& box, float left, float top, float right, float bottom) {
    return D2D1::RectF(
        phosphorPoint(box, left, top).x,
        phosphorPoint(box, left, top).y,
        phosphorPoint(box, right, bottom).x,
        phosphorPoint(box, right, bottom).y);
}

float phosphorSize(const D2D1_RECT_F& box, float value) {
    return ((box.right - box.left) / 256.0f) * value;
}

void drawPhosphorPlusGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color, float size) {
    // Based on @phosphor-icons/core plus-bold.svg, simplified to two rounded bars.
    const D2D1_RECT_F box = centeredIconBox(rect, size);
    const float radius = phosphorSize(box, 12.0f);
    render.fillRoundedRect(
        D2D1::RoundedRect(phosphorRect(box, 28.0f, 116.0f, 228.0f, 140.0f), radius, radius),
        color);
    render.fillRoundedRect(
        D2D1::RoundedRect(phosphorRect(box, 116.0f, 28.0f, 140.0f, 228.0f), radius, radius),
        color);
}

void drawPhosphorCaretGlyph(RenderContext& render, const D2D1_RECT_F& rect, bool left, D2D1_COLOR_F color) {
    // Based on @phosphor-icons/core caret-left/right-bold.svg, with rounded joins approximated by polygon edges.
    const D2D1_RECT_F box = centeredIconBox(rect, 17.0f);
    if (left) {
        const std::array<D2D1_POINT_2F, 7> points = {
            phosphorPoint(box, 160.0f, 39.5f),
            phosphorPoint(box, 177.0f, 56.5f),
            phosphorPoint(box, 105.5f, 128.0f),
            phosphorPoint(box, 177.0f, 199.5f),
            phosphorPoint(box, 160.0f, 216.5f),
            phosphorPoint(box, 71.5f, 136.5f),
            phosphorPoint(box, 71.5f, 119.5f),
        };
        render.fillPolygon(points, color);
    } else {
        const std::array<D2D1_POINT_2F, 7> points = {
            phosphorPoint(box, 96.0f, 39.5f),
            phosphorPoint(box, 184.5f, 119.5f),
            phosphorPoint(box, 184.5f, 136.5f),
            phosphorPoint(box, 96.0f, 216.5f),
            phosphorPoint(box, 79.0f, 199.5f),
            phosphorPoint(box, 150.5f, 128.0f),
            phosphorPoint(box, 79.0f, 56.5f),
        };
        render.fillPolygon(points, color);
    }
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
    drawSvgIcon(render, kSvgArrowsDownUp, rect, color, 18.0f);
}

void drawSettingsGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    drawSvgIcon(render, kSvgGearSix, rect, color, 18.0f);
}

void drawNewFolderGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    drawSvgIcon(render, kSvgFolderPlus, rect, color, 20.0f);
}

void drawNewFileGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    drawSvgIcon(render, kSvgFilePlus, rect, color, 20.0f);
}

void drawPlusGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    drawSvgIcon(render, kSvgPlus, rect, color, 13.0f);
}

void drawChevronGlyph(RenderContext& render, const D2D1_RECT_F& rect, bool left, D2D1_COLOR_F color) {
    drawSvgIcon(render, left ? kSvgCaretLeft : kSvgCaretRight, rect, color, 17.0f);
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
    case ToolbarCommand::Search:
        break;
    }
}

void drawSearchGlyph(RenderContext& render, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    const D2D1_RECT_F iconRect = D2D1::RectF(rect.left + 8.0f, rect.top + 5.0f, rect.left + 26.0f, rect.top + 23.0f);
    render.fillSvgPath(kSvgMagnifyingGlass, 256.0f, 256.0f, iconRect, color);
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
        render.fillRoundedRect(
            D2D1::RoundedRect(clippedPathRect, 10.0f, 10.0f),
            withAlpha(tokens.appOverlay, 0.76f));
        render.drawRoundedRect(
            D2D1::RoundedRect(clippedPathRect, 10.0f, 10.0f),
            withAlpha(tokens.appLine, 0.82f),
            1.0f);
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
