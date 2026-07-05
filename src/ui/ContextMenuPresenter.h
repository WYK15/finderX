#pragma once

#include "settings/AppSettings.h"
#include "ui/ThemeTokens.h"

#include <memory>
#include <string>
#include <vector>

#include <windows.h>

namespace finderx::ui {

struct ContextMenuItem {
    UINT command = 0;
    std::wstring text;
    bool separator = false;
    bool checked = false;
};

struct ContextMenuMetrics {
    int itemHeight = 30;
    int separatorHeight = 9;
    int minWidth = 180;
    int horizontalPadding = 14;
    int checkColumnWidth = 24;
};

ContextMenuMetrics contextMenuMetrics(float fontSize);
D2D1_COLOR_F contextMenuHoverColor(ThemeMode mode);
D2D1_COLOR_F contextMenuTextColor(ThemeMode mode, bool selected);

class ContextMenuPresenter {
public:
    ContextMenuPresenter(ThemeMode themeMode, float fontSize, std::wstring fontFamily);
    ~ContextMenuPresenter();

    ContextMenuPresenter(const ContextMenuPresenter&) = delete;
    ContextMenuPresenter& operator=(const ContextMenuPresenter&) = delete;

    bool appendItem(HMENU menu, UINT command, std::wstring text, bool checked = false);
    bool appendSeparator(HMENU menu);
    void track(HWND owner, HMENU menu, POINT screenPoint);
    bool measure(const MEASUREITEMSTRUCT& item, MEASUREITEMSTRUCT& result) const;
    bool draw(const DRAWITEMSTRUCT& item) const;

private:
    struct ItemStorage;

    ContextMenuItem* store(ContextMenuItem item);
    SIZE measureText(HDC dc, const std::wstring& text) const;

    ThemeMode themeMode_;
    ThemeTokens tokens_;
    ContextMenuMetrics metrics_;
    std::wstring fontFamily_;
    HFONT font_ = nullptr;
    HBRUSH backgroundBrush_ = nullptr;
    std::vector<std::unique_ptr<ItemStorage>> items_;
};

} // namespace finderx::ui
