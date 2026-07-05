#include "ui/ContextMenuPresenter.h"

#include <algorithm>
#include <cmath>

namespace finderx::ui {

namespace {

COLORREF colorRef(D2D1_COLOR_F color) {
    const auto channel = [](float value) {
        return static_cast<BYTE>((std::clamp)(value, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    return RGB(channel(color.r), channel(color.g), channel(color.b));
}

int scaled(float value, float fontSize) {
    return static_cast<int>(std::round(value * (std::max)(1.0f, fontSize / 13.0f)));
}

HFONT createMenuFont(const std::wstring& family, float fontSize) {
    LOGFONTW logFont{};
    HDC dc = GetDC(nullptr);
    const int dpiY = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
    if (dc) {
        ReleaseDC(nullptr, dc);
    }
    logFont.lfHeight = -MulDiv(static_cast<int>(std::round(fontSize)), dpiY, 72);
    logFont.lfWeight = FW_NORMAL;
    wcscpy_s(logFont.lfFaceName, family.empty() ? L"Segoe UI" : family.c_str());
    return CreateFontIndirectW(&logFont);
}

} // namespace

struct ContextMenuPresenter::ItemStorage {
    ContextMenuItem item;
};

ContextMenuMetrics contextMenuMetrics(float fontSize) {
    ContextMenuMetrics metrics;
    metrics.itemHeight = (std::max)(28, scaled(30.0f, fontSize));
    metrics.separatorHeight = (std::max)(8, scaled(9.0f, fontSize));
    metrics.minWidth = (std::max)(180, scaled(180.0f, fontSize));
    metrics.horizontalPadding = (std::max)(12, scaled(14.0f, fontSize));
    metrics.checkColumnWidth = (std::max)(22, scaled(24.0f, fontSize));
    return metrics;
}

D2D1_COLOR_F contextMenuHoverColor(ThemeMode mode) {
    return mode == ThemeMode::Dark
        ? D2D1::ColorF(0.18f, 0.32f, 0.52f, 1.0f)
        : D2D1::ColorF(0.78f, 0.88f, 1.0f, 1.0f);
}

D2D1_COLOR_F contextMenuTextColor(ThemeMode mode, bool selected) {
    const ThemeTokens tokens = themeTokens(mode);
    if (!selected) {
        return tokens.ink;
    }
    return mode == ThemeMode::Dark ? D2D1::ColorF(0.96f, 0.98f, 1.0f, 1.0f) : tokens.ink;
}

ContextMenuPresenter::ContextMenuPresenter(ThemeMode themeMode, float fontSize, std::wstring fontFamily)
    : themeMode_(themeMode),
      tokens_(themeTokens(themeMode)),
      metrics_(contextMenuMetrics(fontSize)),
      fontFamily_(std::move(fontFamily)),
      font_(createMenuFont(fontFamily_, fontSize)),
      backgroundBrush_(CreateSolidBrush(colorRef(tokens_.menu))) {}

ContextMenuPresenter::~ContextMenuPresenter() {
    if (font_) {
        DeleteObject(font_);
    }
    if (backgroundBrush_) {
        DeleteObject(backgroundBrush_);
    }
}

ContextMenuItem* ContextMenuPresenter::store(ContextMenuItem item) {
    auto storage = std::make_unique<ItemStorage>();
    storage->item = std::move(item);
    ContextMenuItem* result = &storage->item;
    items_.push_back(std::move(storage));
    return result;
}

bool ContextMenuPresenter::appendItem(HMENU menu, UINT command, std::wstring text, bool checked) {
    ContextMenuItem* item = store(ContextMenuItem{command, std::move(text), false, checked});
    MENUITEMINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_DATA;
    info.fType = MFT_OWNERDRAW;
    info.wID = command;
    info.dwItemData = reinterpret_cast<ULONG_PTR>(item);
    return InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &info) != FALSE;
}

bool ContextMenuPresenter::appendSeparator(HMENU menu) {
    ContextMenuItem* item = store(ContextMenuItem{0, L"", true, false});
    MENUITEMINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_FTYPE | MIIM_DATA;
    info.fType = MFT_OWNERDRAW | MFT_SEPARATOR;
    info.dwItemData = reinterpret_cast<ULONG_PTR>(item);
    return InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &info) != FALSE;
}

void ContextMenuPresenter::track(HWND owner, HMENU menu, POINT screenPoint) {
    MENUINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND;
    info.hbrBack = backgroundBrush_;
    SetMenuInfo(menu, &info);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, owner, nullptr);
}

SIZE ContextMenuPresenter::measureText(HDC dc, const std::wstring& text) const {
    SIZE size{};
    if (font_) {
        SelectObject(dc, font_);
    }
    GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size);
    return size;
}

bool ContextMenuPresenter::measure(const MEASUREITEMSTRUCT& item, MEASUREITEMSTRUCT& result) const {
    const auto* menuItem = reinterpret_cast<const ContextMenuItem*>(item.itemData);
    if (!menuItem) {
        return false;
    }
    result = item;
    if (menuItem->separator) {
        result.itemHeight = metrics_.separatorHeight;
        result.itemWidth = metrics_.minWidth;
        return true;
    }

    HDC dc = GetDC(nullptr);
    const SIZE textSize = dc ? measureText(dc, menuItem->text) : SIZE{};
    if (dc) {
        ReleaseDC(nullptr, dc);
    }
    result.itemHeight = metrics_.itemHeight;
    const int measuredWidth = static_cast<int>(textSize.cx)
        + metrics_.checkColumnWidth
        + metrics_.horizontalPadding * 2;
    result.itemWidth = static_cast<UINT>((std::max)(metrics_.minWidth, measuredWidth));
    return true;
}

bool ContextMenuPresenter::draw(const DRAWITEMSTRUCT& item) const {
    const auto* menuItem = reinterpret_cast<const ContextMenuItem*>(item.itemData);
    if (!menuItem || !item.hDC) {
        return false;
    }

    const RECT bounds = item.rcItem;
    HBRUSH background = CreateSolidBrush(colorRef(tokens_.menu));
    FillRect(item.hDC, &bounds, background);
    DeleteObject(background);

    if (menuItem->separator) {
        RECT line = bounds;
        const int y = bounds.top + ((bounds.bottom - bounds.top) / 2);
        line.left += metrics_.horizontalPadding;
        line.right -= metrics_.horizontalPadding;
        line.top = y;
        line.bottom = y + 1;
        HBRUSH lineBrush = CreateSolidBrush(colorRef(tokens_.menuLine));
        FillRect(item.hDC, &line, lineBrush);
        DeleteObject(lineBrush);
        return true;
    }

    const bool selected = (item.itemState & ODS_SELECTED) != 0;
    if (selected) {
        RECT hover = bounds;
        hover.left += 4;
        hover.right -= 4;
        hover.top += 2;
        hover.bottom -= 2;
        HBRUSH hoverBrush = CreateSolidBrush(colorRef(contextMenuHoverColor(themeMode_)));
        FillRect(item.hDC, &hover, hoverBrush);
        DeleteObject(hoverBrush);
    }

    const int oldBkMode = SetBkMode(item.hDC, TRANSPARENT);
    const COLORREF oldTextColor = SetTextColor(item.hDC, colorRef(contextMenuTextColor(themeMode_, selected)));
    HGDIOBJ oldFont = font_ ? SelectObject(item.hDC, font_) : nullptr;

    if (menuItem->checked) {
        RECT checkRect = bounds;
        checkRect.left += metrics_.horizontalPadding - 2;
        checkRect.right = checkRect.left + metrics_.checkColumnWidth;
        DrawTextW(item.hDC, L"\u2713", -1, &checkRect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    }

    RECT textRect = bounds;
    textRect.left += metrics_.horizontalPadding + metrics_.checkColumnWidth;
    textRect.right -= metrics_.horizontalPadding;
    DrawTextW(item.hDC, menuItem->text.c_str(), -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    if (oldFont) {
        SelectObject(item.hDC, oldFont);
    }
    SetTextColor(item.hDC, oldTextColor);
    SetBkMode(item.hDC, oldBkMode);
    return true;
}

} // namespace finderx::ui
