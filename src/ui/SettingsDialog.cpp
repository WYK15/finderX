#include "ui/SettingsDialog.h"
#include "ui/Localization.h"
#include "ui/NativeTitleBar.h"
#include "ui/ThemeTokens.h"
#include "ui/ToolbarCustomization.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <set>
#include <vector>

#include <commdlg.h>

namespace finderx::ui {

namespace {

struct SettingsDialogState {
    SettingsDialogValues values;
    AppSettings initialSettings;
    AppSettings resultSettings;
    std::wstring currentFolder;
    int selectedPage = 0;
    bool accepted = false;
    HBRUSH backgroundBrush = nullptr;
    HBRUSH inputBrush = nullptr;
    HBRUSH panelBrush = nullptr;
    HFONT bodyFont = nullptr;
    HFONT titleFont = nullptr;
    HWND titleControl = nullptr;
    HWND subtitleControl = nullptr;
    std::vector<HWND> panelControls;
    std::vector<HWND> framedControls;
    std::vector<HWND> appearanceControls;
    std::vector<HWND> startupControls;
    std::vector<HWND> filesControls;
    std::vector<HWND> shortcutsControls;
    std::vector<HWND> toolbarControls;
    std::vector<HWND> contextMenuControls;
};

constexpr wchar_t kDialogClassName[] = L"FinderXSettingsDialog";
constexpr int kCategoryListId = 90;
constexpr int kFontEditId = 101;
constexpr int kIconEditId = 102;
constexpr int kThemeComboId = 103;
constexpr int kFontFamilyComboId = 104;
constexpr int kShowHiddenAndSystemId = 105;
constexpr int kWindowWidthEditId = 106;
constexpr int kWindowHeightEditId = 107;
constexpr int kRememberWindowSizeId = 108;
constexpr int kStartupFolderEditId = 109;
constexpr int kUseCurrentFolderId = 110;
constexpr int kContextMenuFontEditId = 111;
constexpr int kItemPaddingEditId = 112;
constexpr int kFavoritesListId = 201;
constexpr int kFavoriteLabelEditId = 202;
constexpr int kFavoritePathEditId = 203;
constexpr int kFavoriteUpId = 204;
constexpr int kFavoriteDownId = 205;
constexpr int kFavoriteRemoveId = 206;
constexpr int kContextToolListId = 301;
constexpr int kContextToolLabelEditId = 302;
constexpr int kContextToolExeEditId = 303;
constexpr int kContextToolArgsEditId = 304;
constexpr int kContextToolFilesId = 305;
constexpr int kContextToolFoldersId = 306;
constexpr int kContextToolAddId = 307;
constexpr int kContextToolUpdateId = 308;
constexpr int kContextToolRemoveId = 309;
constexpr int kContextToolUpId = 310;
constexpr int kContextToolDownId = 311;
constexpr int kContextToolBrowseId = 312;
constexpr int kContextToolVsCodeId = 313;
constexpr int kToolbarAvailableListId = 401;
constexpr int kToolbarListId = 402;
constexpr int kToolbarAddId = 403;
constexpr int kToolbarRemoveId = 404;
constexpr int kToolbarUpId = 405;
constexpr int kToolbarDownId = 406;
constexpr int kLanguageComboId = 407;
constexpr int kOkId = IDOK;
constexpr int kCancelId = IDCANCEL;
constexpr int kDialogWidth = 940;
constexpr int kDialogHeight = 720;
constexpr int kDialogMinWidth = 840;
constexpr int kDialogMinHeight = 560;
constexpr wchar_t kSettingsUiFontFamily[] = L"Segoe UI";
constexpr float kSettingsBodyFontSize = 10.5f;
constexpr float kSettingsTitleFontSize = 13.0f;
constexpr int kPanelRadius = 8;
constexpr int kDialogMargin = 24;
constexpr int kDialogTop = 78;
constexpr int kDialogSidebarWidth = 180;
constexpr int kDialogGap = 20;
constexpr int kDialogActionHeight = 58;
constexpr wchar_t kOriginalComboProcProp[] = L"FinderXOriginalComboProc";

bool hasStyle(DWORD style, DWORD flag) {
    return (style & flag) == flag;
}

UINT dpiForWindow(HWND hwnd) {
    UINT dpi = hwnd ? GetDpiForWindow(hwnd) : GetDpiForSystem();
    return dpi == 0 ? 96 : dpi;
}

int scaleForDpi(UINT dpi, int value) {
    return MulDiv(value, static_cast<int>(dpi), 96);
}

int sx(HWND hwnd, int value) {
    return scaleForDpi(dpiForWindow(hwnd), value);
}

int sy(HWND hwnd, int value) {
    return scaleForDpi(dpiForWindow(hwnd), value);
}

int settingsDialogWidthForDpi(UINT dpi) {
    return scaleForDpi(dpi, kDialogWidth);
}

int settingsDialogHeightForDpi(UINT dpi) {
    return scaleForDpi(dpi, kDialogHeight);
}

int settingsDialogMinWidthForDpi(UINT dpi) {
    return scaleForDpi(dpi, kDialogMinWidth);
}

int settingsDialogMinHeightForDpi(UINT dpi) {
    return scaleForDpi(dpi, kDialogMinHeight);
}

COLORREF colorRef(D2D1_COLOR_F color) {
    const auto channel = [](float value) {
        return static_cast<BYTE>((std::clamp)(value, 0.0f, 1.0f) * 255.0f);
    };
    return RGB(channel(color.r), channel(color.g), channel(color.b));
}

bool isInputControl(HWND control) {
    if (!control) {
        return false;
    }
    const int id = GetDlgCtrlID(control);
    return id == kFontEditId
        || id == kContextMenuFontEditId
        || id == kIconEditId
        || id == kItemPaddingEditId
        || id == kWindowWidthEditId
        || id == kWindowHeightEditId
        || id == kStartupFolderEditId
        || id == kFavoriteLabelEditId
        || id == kFavoritePathEditId
        || id == kContextToolListId
        || id == kContextToolLabelEditId
        || id == kContextToolExeEditId
        || id == kContextToolArgsEditId
        || id == kFavoritesListId
        || id == kToolbarAvailableListId
        || id == kToolbarListId
        || id == kThemeComboId
        || id == kLanguageComboId
        || id == kFontFamilyComboId;
}

bool isOwnerDrawButtonId(int id) {
    switch (id) {
    case kUseCurrentFolderId:
    case kFavoriteUpId:
    case kFavoriteDownId:
    case kFavoriteRemoveId:
    case kContextToolAddId:
    case kContextToolUpdateId:
    case kContextToolRemoveId:
    case kContextToolUpId:
    case kContextToolDownId:
    case kContextToolBrowseId:
    case kContextToolVsCodeId:
    case kToolbarAddId:
    case kToolbarRemoveId:
    case kToolbarUpId:
    case kToolbarDownId:
    case kOkId:
    case kCancelId:
        return true;
    default:
        return false;
    }
}

bool isThemedListId(int id) {
    return id == kCategoryListId
        || id == kFavoritesListId
        || id == kContextToolListId
        || id == kToolbarAvailableListId
        || id == kToolbarListId;
}

void showControls(const std::vector<HWND>& controls, bool visible) {
    for (HWND control : controls) {
        if (control) {
            ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
        }
    }
}

void showSettingsPage(SettingsDialogState& state, int page) {
    state.selectedPage = page;
    showControls(state.appearanceControls, page == 0);
    showControls(state.startupControls, page == 1);
    showControls(state.filesControls, page == 2);
    showControls(state.shortcutsControls, page == 3);
    showControls(state.toolbarControls, page == 4);
    showControls(state.contextMenuControls, page == 5);
    showControls(state.panelControls, false);
}

HFONT createDialogFont(const std::wstring& family, float size, UINT dpi, int weight = FW_NORMAL) {
    LOGFONTW logFont{};
    logFont.lfHeight = -MulDiv(static_cast<int>(std::round(size * 10.0f)), static_cast<int>(dpi), 720);
    logFont.lfWeight = weight;
    wcscpy_s(logFont.lfFaceName, family.empty() ? L"Segoe UI" : family.c_str());
    return CreateFontIndirectW(&logFont);
}

void ensureThemeBrushes(SettingsDialogState& state) {
    if (state.backgroundBrush && state.inputBrush && state.panelBrush) {
        return;
    }
    const ThemeTokens tokens = themeTokens(state.resultSettings.themeMode);
    state.backgroundBrush = CreateSolidBrush(colorRef(tokens.app));
    state.inputBrush = CreateSolidBrush(colorRef(tokens.appInput));
    state.panelBrush = CreateSolidBrush(colorRef(tokens.appOverlay));
}

void deleteThemeBrushes(SettingsDialogState& state) {
    if (state.backgroundBrush) {
        DeleteObject(state.backgroundBrush);
        state.backgroundBrush = nullptr;
    }
    if (state.inputBrush) {
        DeleteObject(state.inputBrush);
        state.inputBrush = nullptr;
    }
    if (state.panelBrush) {
        DeleteObject(state.panelBrush);
        state.panelBrush = nullptr;
    }
    if (state.bodyFont) {
        DeleteObject(state.bodyFont);
        state.bodyFont = nullptr;
    }
    if (state.titleFont) {
        DeleteObject(state.titleFont);
        state.titleFont = nullptr;
    }
}

std::wstring shortcutHelpText(LanguageMode languageMode) {
    if (languageMode == LanguageMode::English) {
        return L"Keyboard shortcuts:\r\n"
               L"Ctrl+T  New tab\r\n"
               L"Ctrl+W  Close tab\r\n"
               L"Ctrl+F  Search\r\n"
               L"F5 / Ctrl+R  Refresh\r\n"
               L"F2  Rename\r\n"
               L"Delete  Move to Trash\r\n"
               L"Ctrl+C  Copy\r\n"
               L"Ctrl+X  Cut\r\n"
               L"Ctrl+V  Paste";
    }
    return L"键盘快捷键:\r\n"
           L"Ctrl+T  新建标签页\r\n"
           L"Ctrl+W  关闭标签页\r\n"
           L"Ctrl+F  搜索\r\n"
           L"F5 / Ctrl+R  刷新\r\n"
           L"F2  重命名\r\n"
           L"Delete  移到回收站\r\n"
           L"Ctrl+C  复制\r\n"
           L"Ctrl+X  剪切\r\n"
           L"Ctrl+V  粘贴";
}

void setControlFont(HWND control, HWND parent) {
    auto* state = reinterpret_cast<SettingsDialogState*>(GetWindowLongPtrW(parent, GWLP_USERDATA));
    HFONT font = state && state->bodyFont ? state->bodyFont : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void drawComboArrow(HWND combo) {
    HWND parent = GetParent(combo);
    auto* state = reinterpret_cast<SettingsDialogState*>(GetWindowLongPtrW(parent, GWLP_USERDATA));
    if (!state) {
        return;
    }

    const ThemeTokens tokens = themeTokens(state->resultSettings.themeMode);
    RECT rect{};
    GetClientRect(combo, &rect);
    const int arrowW = sx(combo, 28);
    RECT arrowRect{(std::max)(rect.left, rect.right - arrowW), rect.top, rect.right, rect.bottom};

    HDC dc = GetDC(combo);
    if (!dc) {
        return;
    }

    HBRUSH fill = CreateSolidBrush(colorRef(tokens.appInput));
    FillRect(dc, &arrowRect, fill);
    DeleteObject(fill);

    HPEN line = CreatePen(PS_SOLID, 1, colorRef(tokens.appLine));
    HGDIOBJ oldPen = SelectObject(dc, line);
    MoveToEx(dc, arrowRect.left, arrowRect.top + 1, nullptr);
    LineTo(dc, arrowRect.left, arrowRect.bottom - 1);

    const int centerX = (arrowRect.left + arrowRect.right) / 2;
    const int centerY = (arrowRect.top + arrowRect.bottom) / 2 + sy(combo, 1);
    POINT triangle[] = {
        {centerX - sx(combo, 5), centerY - sy(combo, 3)},
        {centerX + sx(combo, 5), centerY - sy(combo, 3)},
        {centerX, centerY + sy(combo, 4)},
    };
    HBRUSH arrow = CreateSolidBrush(colorRef(tokens.ink));
    HGDIOBJ oldBrush = SelectObject(dc, arrow);
    Polygon(dc, triangle, 3);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(arrow);
    DeleteObject(line);
    ReleaseDC(combo, dc);
}

LRESULT CALLBACK themedComboProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto original = reinterpret_cast<WNDPROC>(GetPropW(hwnd, kOriginalComboProcProp));
    if (!original) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    if (message == WM_NCDESTROY) {
        RemovePropW(hwnd, kOriginalComboProcProp);
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
        return CallWindowProcW(original, hwnd, message, wParam, lParam);
    }

    const LRESULT result = CallWindowProcW(original, hwnd, message, wParam, lParam);
    if (message == WM_PAINT || message == WM_ENABLE || message == WM_SETTEXT || message == CB_SETCURSEL) {
        drawComboArrow(hwnd);
    }
    return result;
}

void subclassComboBox(HWND combo) {
    if (!combo || GetPropW(combo, kOriginalComboProcProp)) {
        return;
    }
    auto original = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(combo, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(themedComboProc)));
    SetPropW(combo, kOriginalComboProcProp, reinterpret_cast<HANDLE>(original));
}

HWND createDialogControl(DWORD exStyle,
                         const wchar_t* className,
                         const wchar_t* text,
                         DWORD style,
                         int x,
                         int y,
                         int width,
                         int height,
                         HWND parent,
                         int id) {
    const bool button = wcscmp(className, L"BUTTON") == 0;
    const bool groupBox = button && hasStyle(style, BS_GROUPBOX);
    const bool ownerDrawButton = button && !groupBox && !hasStyle(style, BS_AUTOCHECKBOX);
    const bool listBox = wcscmp(className, L"LISTBOX") == 0;
    const bool comboBox = wcscmp(className, L"COMBOBOX") == 0;
    const bool editable = wcscmp(className, L"EDIT") == 0 || comboBox;

    if (editable) {
        exStyle &= ~WS_EX_CLIENTEDGE;
        style |= WS_BORDER;
    }
    if (ownerDrawButton) {
        style |= BS_OWNERDRAW;
    }
    if (listBox) {
        exStyle &= ~WS_EX_CLIENTEDGE;
        style &= ~WS_BORDER;
        style |= settingsDialogListBoxStyle();
    }
    if (comboBox) {
        style |= settingsDialogComboBoxStyle();
    }

    const wchar_t* actualClass = groupBox ? L"STATIC" : className;
    DWORD actualStyle = groupBox ? WS_CHILD : style;
    HWND control = CreateWindowExW(exStyle,
                                   actualClass,
                                   text,
                                   actualStyle,
                                   x,
                                   y,
                                   width,
                                   height,
                                   parent,
                                   id == 0 ? nullptr : reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandleW(nullptr),
                                   nullptr);
    if (control) {
        setControlFont(control, parent);
        if (comboBox) {
            subclassComboBox(control);
        }
    }
    return control;
}

RECT childRect(HWND parent, HWND child) {
    RECT rect{};
    GetWindowRect(child, &rect);
    MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rect), 2);
    return rect;
}

bool isTopHeaderControl(HWND hwnd, HWND control) {
    if (!control) {
        return false;
    }
    RECT rect = childRect(hwnd, control);
    return rect.top < kDialogTop;
}

void drawPanelFrame(HDC dc, HWND hwnd, HWND panel, const ThemeTokens& tokens) {
    if (!panel) {
        return;
    }
    RECT rect = childRect(hwnd, panel);
    HBRUSH fill = CreateSolidBrush(colorRef(tokens.appOverlay));
    HPEN line = CreatePen(PS_SOLID, 1, colorRef(tokens.appLine));
    HGDIOBJ oldBrush = SelectObject(dc, fill);
    HGDIOBJ oldPen = SelectObject(dc, line);
    const int radius = sx(hwnd, kPanelRadius);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(line);
    DeleteObject(fill);

    wchar_t text[128]{};
    GetWindowTextW(panel, text, static_cast<int>(std::size(text)));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, colorRef(tokens.inkDull));
    RECT textRect{rect.left + sx(hwnd, 14), rect.top + sy(hwnd, 8), rect.right - sx(hwnd, 14), rect.top + sy(hwnd, 30)};
    DrawTextW(dc, text, -1, &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
}

void drawControlFrame(HDC dc, HWND hwnd, HWND control, const ThemeTokens& tokens) {
    if (!control || !IsWindowVisible(control)) {
        return;
    }
    RECT rect = childRect(hwnd, control);
    if (GetDlgCtrlID(control) != kCategoryListId) {
        InflateRect(&rect, 1, 1);
    }
    HPEN line = CreatePen(PS_SOLID, 1, colorRef(tokens.appLine));
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    HGDIOBJ oldPen = SelectObject(dc, line);
    const int radius = sx(hwnd, 6);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(line);
}

void paintSettingsDialog(HWND hwnd, SettingsDialogState& state, HDC dc) {
    ensureThemeBrushes(state);
    const ThemeTokens tokens = themeTokens(state.resultSettings.themeMode);
    RECT client{};
    GetClientRect(hwnd, &client);
    FillRect(dc, &client, state.backgroundBrush);

    for (std::size_t index = 0; index < state.panelControls.size(); ++index) {
        const bool visible = (index == 0 && state.selectedPage == 0)
            || (index == 1 && state.selectedPage == 1)
            || ((index == 2 || index == 3) && state.selectedPage == 2)
            || (index == 4 && state.selectedPage == 3)
            || (index == 5 && state.selectedPage == 4);
        if (visible) {
            drawPanelFrame(dc, hwnd, state.panelControls[index], tokens);
        }
    }
    for (HWND control : state.framedControls) {
        drawControlFrame(dc, hwnd, control, tokens);
    }
}

void moveControl(HWND hwnd, int id, int x, int y, int width, int height) {
    HWND control = GetDlgItem(hwnd, id);
    if (control) {
        SetWindowPos(control, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void moveComboControl(HWND hwnd, int id, int x, int y, int width, int closedHeight, int visibleItems) {
    HWND control = GetDlgItem(hwnd, id);
    if (control) {
        const int itemHeight = static_cast<int>(SendMessageW(control, CB_GETITEMHEIGHT, 0, 0));
        const int droppedHeight = settingsDialogComboDroppedHeight(itemHeight > 0 ? itemHeight : closedHeight, visibleItems);
        SetWindowPos(control, nullptr, x, y, width, droppedHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void moveHandle(HWND control, int x, int y, int width, int height) {
    if (control) {
        SetWindowPos(control, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void layoutSettingsDialog(HWND hwnd, SettingsDialogState& state) {
    RECT client{};
    GetClientRect(hwnd, &client);
    const UINT dpi = dpiForWindow(hwnd);
    const auto sc = [dpi](int value) { return scaleForDpi(dpi, value); };
    const int width = (std::max)(static_cast<int>(client.right - client.left), settingsDialogMinWidthForDpi(dpi));
    const int height = (std::max)(static_cast<int>(client.bottom - client.top), settingsDialogMinHeightForDpi(dpi));
    const int left = sc(kDialogMargin);
    const int top = sc(kDialogTop);
    const int actionTop = height - sc(kDialogActionHeight);
    const int contentBottom = actionTop - sc(18);
    const int sidebarWidth = sc(kDialogSidebarWidth);
    const int contentLeft = left + sidebarWidth + sc(kDialogGap);
    const int contentRight = width - sc(kDialogMargin);
    const int contentWidth = (std::max)(sc(360), contentRight - contentLeft);
    const int contentHeight = (std::max)(sc(260), contentBottom - top);
    const int panelX = contentLeft;
    const int panelY = top;
    const int panelW = contentWidth;
    const int panelH = contentHeight;
    const int innerX = panelX + sc(24);
    const int labelW = sc(170);
    const int inputX = innerX + labelW + sc(18);
    const int inputW = (std::max)(sc(120), panelX + panelW - inputX - sc(24));
    const int rowH = sc(24);
    const int labelH = sc(26);
    const int rowGap = sc(48);
    const int buttonH = sc(30);

    moveHandle(state.titleControl, sc(24), sc(18), width - sc(48), sc(28));
    moveHandle(state.subtitleControl, sc(24), sc(48), width - sc(48), sc(26));

    moveControl(hwnd, kCategoryListId, left, top, sidebarWidth, panelH);
    moveControl(hwnd, kOkId, width - sc(196), actionTop + sc(18), sc(76), buttonH);
    moveControl(hwnd, kCancelId, width - sc(108), actionTop + sc(18), sc(84), buttonH);

    if (state.panelControls.size() >= 7) {
        moveHandle(state.panelControls[0], panelX, panelY, panelW, panelH);
        moveHandle(state.panelControls[1], panelX, panelY, panelW, panelH);
        moveHandle(state.panelControls[2], panelX, panelY, panelW, sc(92));
        moveHandle(state.panelControls[3], panelX, panelY + sc(112), panelW, panelH - sc(112));
        moveHandle(state.panelControls[4], panelX, panelY, panelW, panelH);
        moveHandle(state.panelControls[5], panelX, panelY, panelW, panelH);
        moveHandle(state.panelControls[6], panelX, panelY, panelW, panelH);
    }

    if (state.appearanceControls.size() >= 15) {
        const int y0 = panelY + sc(38);
        moveHandle(state.appearanceControls[1], innerX, y0 + sc(3), labelW, labelH);
        moveComboControl(hwnd, kFontFamilyComboId, inputX, y0, (std::min)(inputW, sc(320)), rowH + sc(4), 16);
        moveHandle(state.appearanceControls[3], innerX, y0 + rowGap + sc(3), labelW, labelH);
        moveControl(hwnd, kFontEditId, inputX, y0 + rowGap, sc(130), rowH);
        moveHandle(state.appearanceControls[5], innerX, y0 + rowGap * 2 + sc(3), labelW, labelH);
        moveControl(hwnd, kContextMenuFontEditId, inputX, y0 + rowGap * 2, sc(130), rowH);
        moveHandle(state.appearanceControls[7], innerX, y0 + rowGap * 3 + sc(3), labelW, labelH);
        moveControl(hwnd, kIconEditId, inputX, y0 + rowGap * 3, sc(130), rowH);
        moveHandle(state.appearanceControls[9], innerX, y0 + rowGap * 4 + sc(3), labelW, labelH);
        moveControl(hwnd, kItemPaddingEditId, inputX, y0 + rowGap * 4, sc(130), rowH);
        moveHandle(state.appearanceControls[11], innerX, y0 + rowGap * 5 + sc(3), labelW, labelH);
        moveComboControl(hwnd, kThemeComboId, inputX, y0 + rowGap * 5, sc(160), rowH + sc(4), 4);
        moveHandle(state.appearanceControls[13], innerX, y0 + rowGap * 6 + sc(3), labelW, labelH);
        moveComboControl(hwnd, kLanguageComboId, inputX, y0 + rowGap * 6, sc(160), rowH + sc(4), 4);
    }

    if (state.startupControls.size() >= 9) {
        const int y0 = panelY + sc(38);
        moveHandle(state.startupControls[1], innerX, y0 + sc(3), labelW, labelH);
        moveControl(hwnd, kWindowWidthEditId, inputX, y0, sc(142), rowH);
        moveHandle(state.startupControls[3], innerX, y0 + rowGap + sc(3), labelW, labelH);
        moveControl(hwnd, kWindowHeightEditId, inputX, y0 + rowGap, sc(142), rowH);
        moveControl(hwnd, kRememberWindowSizeId, innerX, y0 + rowGap * 2, (std::min)(panelW - sc(48), sc(460)), rowH);

        const int folderY = y0 + rowGap * 3 + sc(10);
        const int buttonW = sc(126);
        moveHandle(state.startupControls[6], innerX, folderY + sc(3), labelW, labelH);
        moveControl(hwnd, kStartupFolderEditId, inputX, folderY, (std::max)(sc(180), inputW - buttonW - sc(12)), rowH);
        moveControl(hwnd, kUseCurrentFolderId, panelX + panelW - sc(24) - buttonW, folderY, buttonW, buttonH);
    }

    if (state.filesControls.size() >= 12) {
        moveControl(hwnd, kShowHiddenAndSystemId, innerX, panelY + sc(38), (std::min)(panelW - sc(48), sc(460)), rowH);
        const int favY = panelY + sc(148);
        const int listW = (std::min)(sc(320), (std::max)(sc(230), panelW * 38 / 100));
        const int formX = innerX + listW + sc(28);
        const int formW = (std::max)(sc(260), panelX + panelW - formX - sc(24));
        moveHandle(state.filesControls[3], innerX, favY + sc(28), listW, labelH);
        moveControl(hwnd, kFavoritesListId, innerX, favY + sc(60), listW, (std::max)(sc(150), panelH - sc(214)));
        moveHandle(state.filesControls[5], formX, favY + sc(50), sc(190), labelH);
        moveControl(hwnd, kFavoriteLabelEditId, formX, favY + sc(82), formW, rowH);
        moveHandle(state.filesControls[7], formX, favY + sc(132), sc(190), labelH);
        moveControl(hwnd, kFavoritePathEditId, formX, favY + sc(164), formW, rowH);
        moveControl(hwnd, kFavoriteUpId, formX, favY + sc(214), sc(74), buttonH);
        moveControl(hwnd, kFavoriteDownId, formX + sc(86), favY + sc(214), sc(88), buttonH);
        moveControl(hwnd, kFavoriteRemoveId, formX + sc(186), favY + sc(214), sc(104), buttonH);
    }

    if (state.shortcutsControls.size() >= 2) {
        moveHandle(state.shortcutsControls[1], innerX, panelY + sc(38), panelW - sc(48), panelH - sc(72));
    }

    if (state.toolbarControls.size() >= 9) {
        const int buttonW = sc(108);
        const int buttonX = panelX + panelW / 2 - buttonW / 2;
        const int listW = (std::max)(sc(190), (panelW - sc(48) - buttonW - sc(48)) / 2);
        const int leftX = innerX;
        const int rightX = panelX + panelW - sc(24) - listW;
        const int listTop = panelY + sc(82);
        const int listH = (std::max)(sc(180), panelH - sc(148));
        moveHandle(state.toolbarControls[1], leftX, panelY + sc(44), listW, labelH);
        moveControl(hwnd, kToolbarAvailableListId, leftX, listTop, listW, listH);
        moveHandle(state.toolbarControls[3], rightX, panelY + sc(44), listW, labelH);
        moveControl(hwnd, kToolbarListId, rightX, listTop, listW, listH);
        moveControl(hwnd, kToolbarAddId, buttonX, listTop + sc(26), buttonW, buttonH);
        moveControl(hwnd, kToolbarRemoveId, buttonX, listTop + sc(64), buttonW, buttonH);
        moveControl(hwnd, kToolbarUpId, buttonX, listTop + sc(132), buttonW, buttonH);
        moveControl(hwnd, kToolbarDownId, buttonX, listTop + sc(170), buttonW, buttonH);
    }

    if (state.contextMenuControls.size() >= 17) {
        const int buttonY = panelY + panelH - sc(52);
        const int listW = (std::min)(sc(320), (std::max)(sc(230), panelW * 38 / 100));
        moveControl(hwnd, kContextToolListId, innerX, panelY + sc(38), listW, (std::max)(sc(150), buttonY - panelY - sc(56)));
        const int formX = innerX + listW + sc(28);
        const int formW = (std::max)(sc(280), panelX + panelW - formX - sc(24));
        const int browseW = sc(100);
        moveHandle(state.contextMenuControls[2], formX, panelY + sc(38), sc(200), labelH);
        moveControl(hwnd, kContextToolLabelEditId, formX, panelY + sc(70), formW, rowH);
        moveHandle(state.contextMenuControls[4], formX, panelY + sc(120), sc(200), labelH);
        moveControl(hwnd, kContextToolExeEditId, formX, panelY + sc(152), (std::max)(sc(150), formW - browseW - sc(12)), rowH);
        moveControl(hwnd, kContextToolBrowseId, formX + formW - browseW, panelY + sc(152), browseW, buttonH);
        moveHandle(state.contextMenuControls[7], formX, panelY + sc(202), sc(200), labelH);
        moveControl(hwnd, kContextToolArgsEditId, formX, panelY + sc(234), formW, rowH);
        moveControl(hwnd, kContextToolFilesId, formX, panelY + sc(284), sc(180), rowH);
        moveControl(hwnd, kContextToolFoldersId, formX, panelY + sc(324), sc(200), rowH);
        moveControl(hwnd, kContextToolVsCodeId, formX, panelY + sc(376), sc(120), buttonH);
        moveControl(hwnd, kContextToolAddId, innerX, buttonY, sc(70), buttonH);
        moveControl(hwnd, kContextToolUpdateId, innerX + sc(82), buttonY, sc(88), buttonH);
        moveControl(hwnd, kContextToolRemoveId, innerX + sc(182), buttonY, sc(100), buttonH);
        moveControl(hwnd, kContextToolUpId, innerX + sc(294), buttonY, sc(66), buttonH);
        moveControl(hwnd, kContextToolDownId, innerX + sc(372), buttonY, sc(82), buttonH);
    }

    InvalidateRect(hwnd, nullptr, TRUE);
}

void drawOwnerButton(HWND hwnd, SettingsDialogState& state, const DRAWITEMSTRUCT& item) {
    const ThemeTokens tokens = themeTokens(state.resultSettings.themeMode);
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool focused = (item.itemState & ODS_FOCUS) != 0;
    const bool primary = item.CtlID == kOkId;
    D2D1_COLOR_F fill = primary ? tokens.accent : tokens.appOverlay;
    if (pressed) {
        fill = primary ? tokens.accentDeep : tokens.appActive;
    } else if (focused) {
        fill = primary ? tokens.accent : tokens.appHover;
    }

    HBRUSH fillBrush = CreateSolidBrush(colorRef(fill));
    HPEN linePen = CreatePen(PS_SOLID, 1, colorRef(primary ? tokens.accent : tokens.appLine));
    HGDIOBJ oldBrush = SelectObject(item.hDC, fillBrush);
    HGDIOBJ oldPen = SelectObject(item.hDC, linePen);
    RoundRect(item.hDC, item.rcItem.left, item.rcItem.top, item.rcItem.right, item.rcItem.bottom, 7, 7);
    SelectObject(item.hDC, oldPen);
    SelectObject(item.hDC, oldBrush);
    DeleteObject(linePen);
    DeleteObject(fillBrush);

    wchar_t text[128]{};
    GetWindowTextW(item.hwndItem, text, static_cast<int>(std::size(text)));
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, colorRef(primary ? tokens.inkOnAccent : tokens.ink));
    if (state.bodyFont) {
        SelectObject(item.hDC, state.bodyFont);
    }
    RECT textRect = item.rcItem;
    DrawTextW(item.hDC, text, -1, &textRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
}

void drawOwnerListItem(SettingsDialogState& state, const DRAWITEMSTRUCT& item) {
    const ThemeTokens tokens = themeTokens(state.resultSettings.themeMode);
    HBRUSH background = CreateSolidBrush(colorRef(tokens.appInput));
    FillRect(item.hDC, &item.rcItem, background);
    DeleteObject(background);

    if (item.itemID == static_cast<UINT>(-1)) {
        return;
    }

    const bool selected = (item.itemState & ODS_SELECTED) != 0;
    if (selected) {
        RECT selectedRect = item.rcItem;
        InflateRect(&selectedRect, -3, -2);
        HBRUSH selectedBrush = CreateSolidBrush(colorRef(tokens.rowSelected));
        FillRect(item.hDC, &selectedRect, selectedBrush);
        DeleteObject(selectedBrush);
    }

    const LRESULT length = SendMessageW(item.hwndItem, LB_GETTEXTLEN, item.itemID, 0);
    if (length == LB_ERR || length < 0) {
        return;
    }
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    SendMessageW(item.hwndItem, LB_GETTEXT, item.itemID, reinterpret_cast<LPARAM>(text.data()));
    text.resize(static_cast<std::size_t>(length));

    if (state.bodyFont) {
        SelectObject(item.hDC, state.bodyFont);
    }
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, colorRef(selected ? tokens.inkOnAccent : tokens.ink));
    RECT textRect = item.rcItem;
    textRect.left += 10;
    textRect.right -= 10;
    DrawTextW(item.hDC, text.c_str(), -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
}

void drawOwnerComboItem(SettingsDialogState& state, const DRAWITEMSTRUCT& item) {
    const ThemeTokens tokens = themeTokens(state.resultSettings.themeMode);
    HBRUSH background = CreateSolidBrush(colorRef(tokens.appInput));
    FillRect(item.hDC, &item.rcItem, background);
    DeleteObject(background);

    if (item.itemID == static_cast<UINT>(-1)) {
        return;
    }

    const bool selected = (item.itemState & ODS_SELECTED) != 0;
    if (selected) {
        HBRUSH selectedBrush = CreateSolidBrush(colorRef(tokens.appActive));
        FillRect(item.hDC, &item.rcItem, selectedBrush);
        DeleteObject(selectedBrush);
    }

    const LRESULT length = SendMessageW(item.hwndItem, CB_GETLBTEXTLEN, item.itemID, 0);
    if (length == CB_ERR || length < 0) {
        return;
    }
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    SendMessageW(item.hwndItem, CB_GETLBTEXT, item.itemID, reinterpret_cast<LPARAM>(text.data()));
    text.resize(static_cast<std::size_t>(length));

    if (state.bodyFont) {
        SelectObject(item.hDC, state.bodyFont);
    }
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, colorRef(tokens.ink));
    RECT textRect = item.rcItem;
    textRect.left += sx(item.hwndItem, 10);
    textRect.right -= sx(item.hwndItem, 36);
    DrawTextW(item.hDC, text.c_str(), -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
}

std::wstring getEditText(HWND edit) {
    const int length = GetWindowTextLengthW(edit);
    if (length <= 0) {
        return {};
    }

    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(edit, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return text;
}

std::wstring formatSettingValue(float value) {
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%.1f", value);
    return buffer;
}

bool tryParseFloat(const std::wstring& text, float& value) {
    const wchar_t* start = text.c_str();
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(start, &end);
    if (end == start || !std::isfinite(parsed)) {
        return false;
    }

    while (*end == L' ' || *end == L'\t' || *end == L'\r' || *end == L'\n') {
        ++end;
    }
    if (*end != L'\0') {
        return false;
    }

    value = parsed;
    return true;
}

bool settingsEqualInternal(const AppSettings& left, const AppSettings& right) {
    if (left.fontSize != right.fontSize || left.fontFamily != right.fontFamily
        || left.contextMenuFontSize != right.contextMenuFontSize || left.iconSize != right.iconSize
        || left.itemPadding != right.itemPadding
        || left.windowWidth != right.windowWidth || left.windowHeight != right.windowHeight
        || left.rememberWindowSize != right.rememberWindowSize
        || left.startupFolder != right.startupFolder
        || left.themeMode != right.themeMode || left.languageMode != right.languageMode || left.showHiddenAndSystemItems != right.showHiddenAndSystemItems
        || left.sortColumn != right.sortColumn || left.sortDirection != right.sortDirection
        || left.toolbarCommands != right.toolbarCommands
        || left.favorites.size() != right.favorites.size()
        || left.contextMenuTools.size() != right.contextMenuTools.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.favorites.size(); ++index) {
        if (left.favorites[index].label != right.favorites[index].label
            || left.favorites[index].path != right.favorites[index].path) {
            return false;
        }
    }
    for (std::size_t index = 0; index < left.contextMenuTools.size(); ++index) {
        const ContextMenuTool& lhs = left.contextMenuTools[index];
        const ContextMenuTool& rhs = right.contextMenuTools[index];
        if (lhs.label != rhs.label
            || lhs.executablePath != rhs.executablePath
            || lhs.arguments != rhs.arguments
            || lhs.appliesToFiles != rhs.appliesToFiles
            || lhs.appliesToFolders != rhs.appliesToFolders) {
            return false;
        }
    }
    return true;
}

int CALLBACK collectFontFamily(const LOGFONTW* font, const TEXTMETRICW*, DWORD type, LPARAM param) {
    if ((type & TRUETYPE_FONTTYPE) == 0 && (type & DEVICE_FONTTYPE) != 0) {
        return 1;
    }
    if (!font || font->lfFaceName[0] == L'\0' || font->lfFaceName[0] == L'@') {
        return 1;
    }

    auto* fonts = reinterpret_cast<std::set<std::wstring>*>(param);
    fonts->insert(font->lfFaceName);
    return 1;
}

std::set<std::wstring> installedFontFamilies() {
    std::set<std::wstring> fonts;
    fonts.insert(L"Segoe UI");
    fonts.insert(L"Microsoft YaHei UI");
    fonts.insert(L"Consolas");

    HDC dc = GetDC(nullptr);
    if (dc) {
        LOGFONTW query{};
        query.lfCharSet = DEFAULT_CHARSET;
        EnumFontFamiliesExW(dc, &query, collectFontFamily, reinterpret_cast<LPARAM>(&fonts), 0);
        ReleaseDC(nullptr, dc);
    }
    return fonts;
}

void populateFontFamilyCombo(HWND combo, const std::wstring& selectedFontFamily) {
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);

    const std::set<std::wstring> fonts = installedFontFamilies();
    for (const std::wstring& fontFamily : fonts) {
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontFamily.c_str()));
    }

    LRESULT selected = SendMessageW(combo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(selectedFontFamily.c_str()));
    if (selected == CB_ERR && !selectedFontFamily.empty()) {
        selected = SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(selectedFontFamily.c_str()));
    }
    SendMessageW(combo, CB_SETCURSEL, selected == CB_ERR ? 0 : static_cast<WPARAM>(selected), 0);
}

std::wstring comboSelectedText(HWND combo) {
    const LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (selected == CB_ERR) {
        return {};
    }

    const LRESULT length = SendMessageW(combo, CB_GETLBTEXTLEN, static_cast<WPARAM>(selected), 0);
    if (length <= 0) {
        return {};
    }

    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    SendMessageW(combo, CB_GETLBTEXT, static_cast<WPARAM>(selected), reinterpret_cast<LPARAM>(text.data()));
    text.resize(static_cast<std::size_t>(length));
    return text;
}

void populateFavoritesList(HWND hwnd, const AppSettings& settings, int selectedIndex) {
    HWND list = GetDlgItem(hwnd, kFavoritesListId);
    if (!list) {
        return;
    }

    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const FavoriteLocation& favorite : settings.favorites) {
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(favorite.label.c_str()));
    }

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(settings.favorites.size())) {
        SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
    }
}

int selectedFavoriteIndex(HWND hwnd) {
    HWND list = GetDlgItem(hwnd, kFavoritesListId);
    if (!list) {
        return -1;
    }

    const LRESULT selected = SendMessageW(list, LB_GETCURSEL, 0, 0);
    return selected == LB_ERR ? -1 : static_cast<int>(selected);
}

void selectFavorite(HWND hwnd, SettingsDialogState& state, int index) {
    HWND list = GetDlgItem(hwnd, kFavoritesListId);
    HWND labelEdit = GetDlgItem(hwnd, kFavoriteLabelEditId);
    HWND pathEdit = GetDlgItem(hwnd, kFavoritePathEditId);
    if (!labelEdit || !pathEdit) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(state.resultSettings.favorites.size())) {
        if (list) {
            SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
        }
        SetWindowTextW(labelEdit, L"");
        SetWindowTextW(pathEdit, L"");
        return;
    }

    if (list) {
        SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }

    const FavoriteLocation& favorite = state.resultSettings.favorites[static_cast<std::size_t>(index)];
    SetWindowTextW(labelEdit, favorite.label.c_str());
    SetWindowTextW(pathEdit, favorite.path.c_str());
}

void syncSelectedFavoriteLabel(HWND hwnd, SettingsDialogState& state) {
    const int index = selectedFavoriteIndex(hwnd);
    if (index < 0 || index >= static_cast<int>(state.resultSettings.favorites.size())) {
        return;
    }

    renameFavorite(state.resultSettings,
                   static_cast<std::size_t>(index),
                               getEditText(GetDlgItem(hwnd, kFavoriteLabelEditId)));
}

int selectedContextToolIndex(HWND hwnd) {
    const LRESULT index = SendMessageW(GetDlgItem(hwnd, kContextToolListId), LB_GETCURSEL, 0, 0);
    return index == LB_ERR ? -1 : static_cast<int>(index);
}

void populateContextToolList(HWND hwnd, const AppSettings& settings, int selectedIndex) {
    HWND list = GetDlgItem(hwnd, kContextToolListId);
    if (!list) {
        return;
    }
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const ContextMenuTool& tool : settings.contextMenuTools) {
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tool.label.c_str()));
    }
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(settings.contextMenuTools.size())) {
        SendMessageW(list, LB_SETCURSEL, selectedIndex, 0);
    }
}

void selectContextTool(HWND hwnd, SettingsDialogState& state, int index) {
    if (index < 0 || index >= static_cast<int>(state.resultSettings.contextMenuTools.size())) {
        SetWindowTextW(GetDlgItem(hwnd, kContextToolLabelEditId), L"");
        SetWindowTextW(GetDlgItem(hwnd, kContextToolExeEditId), L"");
        SetWindowTextW(GetDlgItem(hwnd, kContextToolArgsEditId), L"");
        SendMessageW(GetDlgItem(hwnd, kContextToolFilesId), BM_SETCHECK, BST_CHECKED, 0);
        SendMessageW(GetDlgItem(hwnd, kContextToolFoldersId), BM_SETCHECK, BST_CHECKED, 0);
        return;
    }
    SendMessageW(GetDlgItem(hwnd, kContextToolListId), LB_SETCURSEL, index, 0);
    const ContextMenuTool& tool = state.resultSettings.contextMenuTools[static_cast<std::size_t>(index)];
    SetWindowTextW(GetDlgItem(hwnd, kContextToolLabelEditId), tool.label.c_str());
    SetWindowTextW(GetDlgItem(hwnd, kContextToolExeEditId), tool.executablePath.c_str());
    SetWindowTextW(GetDlgItem(hwnd, kContextToolArgsEditId), tool.arguments.c_str());
    SendMessageW(GetDlgItem(hwnd, kContextToolFilesId), BM_SETCHECK, tool.appliesToFiles ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, kContextToolFoldersId), BM_SETCHECK, tool.appliesToFolders ? BST_CHECKED : BST_UNCHECKED, 0);
}

ContextMenuTool contextToolFromFields(HWND hwnd) {
    ContextMenuTool tool;
    tool.label = getEditText(GetDlgItem(hwnd, kContextToolLabelEditId));
    tool.executablePath = getEditText(GetDlgItem(hwnd, kContextToolExeEditId));
    tool.arguments = getEditText(GetDlgItem(hwnd, kContextToolArgsEditId));
    if (tool.arguments.empty()) {
        tool.arguments = L"\"{path}\"";
    }
    tool.appliesToFiles = SendMessageW(GetDlgItem(hwnd, kContextToolFilesId), BM_GETCHECK, 0, 0) == BST_CHECKED;
    tool.appliesToFolders = SendMessageW(GetDlgItem(hwnd, kContextToolFoldersId), BM_GETCHECK, 0, 0) == BST_CHECKED;
    return tool;
}

bool browseExecutable(HWND hwnd, std::wstring& path, LanguageMode languageMode) {
    wchar_t buffer[MAX_PATH]{};
    const std::wstring current = getEditText(GetDlgItem(hwnd, kContextToolExeEditId));
    if (!current.empty() && current.size() < std::size(buffer)) {
        std::copy(current.begin(), current.end(), buffer);
    }

    OPENFILENAMEW openFile{};
    openFile.lStructSize = sizeof(openFile);
    openFile.hwndOwner = hwnd;
    openFile.lpstrFilter = L"Programs (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
    openFile.lpstrFile = buffer;
    openFile.nMaxFile = static_cast<DWORD>(std::size(buffer));
    openFile.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    openFile.lpstrTitle = tr(StringId::ChooseProgram, languageMode).data();
    if (!GetOpenFileNameW(&openFile)) {
        return false;
    }

    path = buffer;
    return !path.empty();
}

void setContextToolTemplate(HWND hwnd, const wchar_t* label, const wchar_t* executableName) {
    SetWindowTextW(GetDlgItem(hwnd, kContextToolLabelEditId), label);
    SetWindowTextW(GetDlgItem(hwnd, kContextToolExeEditId), executableName);
    SetWindowTextW(GetDlgItem(hwnd, kContextToolArgsEditId), L"\"{path}\"");
    SendMessageW(GetDlgItem(hwnd, kContextToolFilesId), BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, kContextToolFoldersId), BM_SETCHECK, BST_CHECKED, 0);
}

void syncSelectedContextTool(HWND hwnd, SettingsDialogState& state) {
    const int index = selectedContextToolIndex(hwnd);
    if (index < 0 || index >= static_cast<int>(state.resultSettings.contextMenuTools.size())) {
        return;
    }
    ContextMenuTool tool = contextToolFromFields(hwnd);
    if (!tool.label.empty() && !tool.executablePath.empty()) {
        state.resultSettings.contextMenuTools[static_cast<std::size_t>(index)] = std::move(tool);
    }
}

int selectedToolbarIndex(HWND hwnd) {
    const LRESULT index = SendMessageW(GetDlgItem(hwnd, kToolbarListId), LB_GETCURSEL, 0, 0);
    return index == LB_ERR ? -1 : static_cast<int>(index);
}

int selectedAvailableToolbarIndex(HWND hwnd) {
    const LRESULT index = SendMessageW(GetDlgItem(hwnd, kToolbarAvailableListId), LB_GETCURSEL, 0, 0);
    return index == LB_ERR ? -1 : static_cast<int>(index);
}

bool hasToolbarCommand(const std::vector<ToolbarCommand>& commands, ToolbarCommand command) {
    return std::find(commands.begin(), commands.end(), command) != commands.end();
}

void populateToolbarCommandList(HWND list, const std::vector<ToolbarCommand>& commands, int selectedIndex, LanguageMode languageMode) {
    if (!list) {
        return;
    }

    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const ToolbarCommand command : commands) {
        const std::wstring label = toolbarCommandLabel(command, languageMode);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(commands.size())) {
        SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
    }
}

void updateToolbarButtons(HWND hwnd, const SettingsDialogState& state) {
    const std::vector<ToolbarCommand> available = availableToolbarCommands();
    const int availableIndex = selectedAvailableToolbarIndex(hwnd);
    const int toolbarIndex = selectedToolbarIndex(hwnd);
    bool canAdd = false;
    if (availableIndex >= 0 && availableIndex < static_cast<int>(available.size())) {
        canAdd = !hasToolbarCommand(state.resultSettings.toolbarCommands, available[static_cast<std::size_t>(availableIndex)]);
    }

    EnableWindow(GetDlgItem(hwnd, kToolbarAddId), canAdd ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwnd, kToolbarRemoveId), toolbarIndex >= 0 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwnd, kToolbarUpId), toolbarIndex > 0 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwnd, kToolbarDownId),
                 toolbarIndex >= 0 && toolbarIndex + 1 < static_cast<int>(state.resultSettings.toolbarCommands.size()) ? TRUE : FALSE);
}

void refreshToolbarLists(HWND hwnd, SettingsDialogState& state, int selectedToolbar = -1) {
    populateToolbarCommandList(GetDlgItem(hwnd, kToolbarAvailableListId), availableToolbarCommands(), selectedAvailableToolbarIndex(hwnd), state.resultSettings.languageMode);
    populateToolbarCommandList(GetDlgItem(hwnd, kToolbarListId), state.resultSettings.toolbarCommands, selectedToolbar, state.resultSettings.languageMode);
    updateToolbarButtons(hwnd, state);
}

void centerDialog(HWND hwnd, HWND owner) {
    RECT dialogRect{};
    GetWindowRect(hwnd, &dialogRect);
    const int dialogWidth = dialogRect.right - dialogRect.left;
    const int dialogHeight = dialogRect.bottom - dialogRect.top;

    RECT anchor{};
    if (owner && GetWindowRect(owner, &anchor)) {
        const int x = anchor.left + ((anchor.right - anchor.left) - dialogWidth) / 2;
        const int y = anchor.top + ((anchor.bottom - anchor.top) - dialogHeight) / 2;
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
        return;
    }

    RECT workArea{};
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) {
        workArea = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    }

    const int x = workArea.left + ((workArea.right - workArea.left) - dialogWidth) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - dialogHeight) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

LRESULT CALLBACK settingsDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<SettingsDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        state = static_cast<SettingsDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    switch (message) {
    case WM_CREATE: {
        if (!state) {
            return -1;
        }
        ensureThemeBrushes(*state);
        const UINT dpi = dpiForWindow(hwnd);
        state->bodyFont = createDialogFont(kSettingsUiFontFamily, settingsDialogBodyFontSize(), dpi);
        state->titleFont = createDialogFont(kSettingsUiFontFamily, settingsDialogTitleFontSize(), dpi, FW_SEMIBOLD);

        HWND title = createDialogControl(0,
                                         L"STATIC",
                                         tr(StringId::FinderXSettings, state->initialSettings.languageMode).data(),
                                         WS_CHILD | WS_VISIBLE,
                                         24,
                                         18,
                                         320,
                                         24,
                                         hwnd,
                                         0);
        if (title && state->titleFont) {
            SendMessageW(title, WM_SETFONT, reinterpret_cast<WPARAM>(state->titleFont), TRUE);
        }
        state->titleControl = title;
        HWND subtitle = createDialogControl(0,
                                            L"STATIC",
                                            tr(StringId::SettingsSubtitle, state->initialSettings.languageMode).data(),
                                            WS_CHILD | WS_VISIBLE,
                                            24,
                                            44,
                                            760,
                                            22,
                                            hwnd,
                                            0);
        state->subtitleControl = subtitle;
        HWND categoryList = createDialogControl(WS_EX_CLIENTEDGE,
                                                L"LISTBOX",
                                                L"",
                                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY,
                                                24,
                                                78,
                                                140,
                                                534,
                                                hwnd,
                                                kCategoryListId);
        if (categoryList) {
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Appearance, state->initialSettings.languageMode).data()));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Startup, state->initialSettings.languageMode).data()));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Files, state->initialSettings.languageMode).data()));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Shortcuts, state->initialSettings.languageMode).data()));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Toolbar, state->initialSettings.languageMode).data()));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::ContextMenu, state->initialSettings.languageMode).data()));
            SendMessageW(categoryList, LB_SETCURSEL, 0, 0);
        }
        HWND appearanceGroup = createDialogControl(0,
                                                   L"BUTTON",
                                                   tr(StringId::Appearance, state->initialSettings.languageMode).data(),
                                                   WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                   190,
                                                   78,
                                                   640,
                                                   534,
                                                   hwnd,
                                                   0);
        HWND fontFamilyLabel = createDialogControl(0,
                                                   L"STATIC",
                                                   tr(StringId::FontFamily, state->initialSettings.languageMode).data(),
                                                   WS_CHILD | WS_VISIBLE,
                                                   214,
                                                   110,
                                                   164,
                                                   22,
                                                   hwnd,
                                                   0);
        HWND fontFamilyCombo = createDialogControl(0,
                                                   L"COMBOBOX",
                                                   L"",
                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                                   392,
                                                   106,
                                                   240,
                                                   160,
                                                   hwnd,
                                                   kFontFamilyComboId);
        if (fontFamilyCombo) {
            populateFontFamilyCombo(fontFamilyCombo, state->initialSettings.fontFamily);
        }
        HWND fontLabel = createDialogControl(0,
                                             L"STATIC",
                                             tr(StringId::TextSize, state->initialSettings.languageMode).data(),
                                             WS_CHILD | WS_VISIBLE,
                                             214,
                                             146,
                                             164,
                                             22,
                                             hwnd,
                                             0);
        HWND fontEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                            L"EDIT",
                                            formatSettingValue(state->initialSettings.fontSize).c_str(),
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                            392,
                                            142,
                                            164,
                                            24,
                                            hwnd,
                                            kFontEditId);
        HWND contextMenuFontLabel = createDialogControl(0,
                                                        L"STATIC",
                                                        tr(StringId::MenuTextSize, state->initialSettings.languageMode).data(),
                                                        WS_CHILD | WS_VISIBLE,
                                                        214,
                                                        182,
                                                        164,
                                                        22,
                                                        hwnd,
                                                        0);
        HWND contextMenuFontEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                       L"EDIT",
                                                       formatSettingValue(state->initialSettings.contextMenuFontSize).c_str(),
                                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                       392,
                                                       178,
                                                       164,
                                                       24,
                                                       hwnd,
                                                       kContextMenuFontEditId);
        HWND iconLabel = createDialogControl(0,
                                             L"STATIC",
                                             tr(StringId::IconSize, state->initialSettings.languageMode).data(),
                                             WS_CHILD | WS_VISIBLE,
                                             214,
                                             218,
                                             164,
                                             22,
                                             hwnd,
                                             0);
        HWND iconEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                            L"EDIT",
                                            formatSettingValue(state->initialSettings.iconSize).c_str(),
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                            392,
                                            214,
                                            164,
                                            24,
                                            hwnd,
                                            kIconEditId);
        HWND itemPaddingLabel = createDialogControl(0,
                                                    L"STATIC",
                                                    tr(StringId::ItemPadding, state->initialSettings.languageMode).data(),
                                                    WS_CHILD | WS_VISIBLE,
                                                    214,
                                                    250,
                                                    164,
                                                    22,
                                                    hwnd,
                                                    0);
        HWND itemPaddingEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                   L"EDIT",
                                                   formatSettingValue(state->initialSettings.itemPadding).c_str(),
                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                   392,
                                                   246,
                                                   164,
                                                   24,
                                                   hwnd,
                                                   kItemPaddingEditId);
        HWND themeLabel = createDialogControl(0,
                                              L"STATIC",
                                              tr(StringId::Theme, state->initialSettings.languageMode).data(),
                                              WS_CHILD | WS_VISIBLE,
                                              214,
                                              286,
                                              164,
                                              22,
                                              hwnd,
                                              0);
        HWND themeCombo = createDialogControl(0,
                                              L"COMBOBOX",
                                              L"",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                              392,
                                              282,
                                              164,
                                              120,
                                              hwnd,
                                              kThemeComboId);
        if (themeCombo) {
            SendMessageW(themeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Light, state->initialSettings.languageMode).data()));
            SendMessageW(themeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Dark, state->initialSettings.languageMode).data()));
            SendMessageW(themeCombo, CB_SETCURSEL, state->initialSettings.themeMode == ThemeMode::Light ? 0 : 1, 0);
        }
        HWND languageLabel = createDialogControl(0,
                                                 L"STATIC",
                                                 tr(StringId::Language, state->initialSettings.languageMode).data(),
                                                 WS_CHILD | WS_VISIBLE,
                                                 214,
                                                 322,
                                                 164,
                                                 22,
                                                 hwnd,
                                                 0);
        HWND languageCombo = createDialogControl(0,
                                                 L"COMBOBOX",
                                                 L"",
                                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                                 392,
                                                 318,
                                                 164,
                                                 120,
                                                 hwnd,
                                                 kLanguageComboId);
        if (languageCombo) {
            SendMessageW(languageCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::Chinese, state->initialSettings.languageMode).data()));
            SendMessageW(languageCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(tr(StringId::English, state->initialSettings.languageMode).data()));
            SendMessageW(languageCombo, CB_SETCURSEL, state->initialSettings.languageMode == LanguageMode::Chinese ? 0 : 1, 0);
        }
        HWND behaviorGroup = createDialogControl(0,
                                                 L"BUTTON",
                                                 tr(StringId::FileBrowsing, state->initialSettings.languageMode).data(),
                                                 WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                 190,
                                                 78,
                                                 640,
                                                 78,
                                                 hwnd,
                                                 0);
        HWND showHiddenAndSystem = createDialogControl(0,
                                                       L"BUTTON",
                                                       tr(StringId::ShowHiddenAndSystem, state->initialSettings.languageMode).data(),
                                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                       214,
                                                       112,
                                                       360,
                                                       22,
                                                       hwnd,
                                                       kShowHiddenAndSystemId);
        if (showHiddenAndSystem) {
            SendMessageW(showHiddenAndSystem,
                         BM_SETCHECK,
                         state->initialSettings.showHiddenAndSystemItems ? BST_CHECKED : BST_UNCHECKED,
                         0);
        }
        HWND windowGroup = createDialogControl(0,
                                               L"BUTTON",
                                               tr(StringId::Startup, state->initialSettings.languageMode).data(),
                                               WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                               190,
                                               78,
                                               640,
                                               534,
                                               hwnd,
                                               0);
        HWND windowWidthLabel = createDialogControl(0,
                                                    L"STATIC",
                                                    tr(StringId::Width, state->initialSettings.languageMode).data(),
                                                    WS_CHILD | WS_VISIBLE,
                                                    214,
                                                    112,
                                                    48,
                                                    22,
                                                    hwnd,
                                                    0);
        HWND windowWidthEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                   L"EDIT",
                                                   std::to_wstring(state->initialSettings.windowWidth).c_str(),
                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                   268,
                                                   108,
                                                   74,
                                                   24,
                                                   hwnd,
                                                   kWindowWidthEditId);
        HWND windowHeightLabel = createDialogControl(0,
                                                     L"STATIC",
                                                     tr(StringId::Height, state->initialSettings.languageMode).data(),
                                                     WS_CHILD | WS_VISIBLE,
                                                     372,
                                                     112,
                                                     52,
                                                     22,
                                                     hwnd,
                                                     0);
        HWND windowHeightEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                    L"EDIT",
                                                    std::to_wstring(state->initialSettings.windowHeight).c_str(),
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                    434,
                                                    108,
                                                    74,
                                                    24,
                                                    hwnd,
                                                    kWindowHeightEditId);
        HWND rememberWindowSize = createDialogControl(0,
                                                      L"BUTTON",
                                                      tr(StringId::RememberWindowSize, state->initialSettings.languageMode).data(),
                                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                      214,
                                                      146,
                                                      340,
                                                      22,
                                                      hwnd,
                                                      kRememberWindowSizeId);
        if (rememberWindowSize) {
            SendMessageW(rememberWindowSize,
                         BM_SETCHECK,
                         state->initialSettings.rememberWindowSize ? BST_CHECKED : BST_UNCHECKED,
                         0);
        }
        HWND startupFolderLabel = createDialogControl(0,
                                                      L"STATIC",
                                                      tr(StringId::StartupFolder, state->initialSettings.languageMode).data(),
                                                      WS_CHILD | WS_VISIBLE,
                                                      214,
                                                      198,
                                                      164,
                                                      22,
                                                      hwnd,
                                                      0);
        HWND startupFolderEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                     L"EDIT",
                                                     state->initialSettings.startupFolder.c_str(),
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                     392,
                                                     194,
                                                     260,
                                                     24,
                                                     hwnd,
                                                     kStartupFolderEditId);
        HWND useCurrentFolder = createDialogControl(0,
                                                    L"BUTTON",
                                                    tr(StringId::UseCurrent, state->initialSettings.languageMode).data(),
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                    664,
                                                    192,
                                                    108,
                                                    28,
                                                    hwnd,
                                                    kUseCurrentFolderId);
        if (useCurrentFolder && state->currentFolder.empty()) {
            EnableWindow(useCurrentFolder, FALSE);
        }
        HWND favoritesGroup = createDialogControl(0,
                                                  L"BUTTON",
                                                  tr(StringId::Favorites, state->initialSettings.languageMode).data(),
                                                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                  190,
                                                  176,
                                                  640,
                                                  436,
                                                  hwnd,
                                                  0);
        HWND favoritesLabel = createDialogControl(0,
                                                  L"STATIC",
                                                  tr(StringId::FavoritesDescription, state->initialSettings.languageMode).data(),
                                                  WS_CHILD | WS_VISIBLE,
                                                  214,
                                                  206,
                                                  260,
                                                  18,
                                                  hwnd,
                                                  0);
        HWND favoritesList = createDialogControl(WS_EX_CLIENTEDGE,
                                                 L"LISTBOX",
                                                 L"",
                                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
                                                 214,
                                                 230,
                                                 180,
                                                 286,
                                                 hwnd,
                                                 kFavoritesListId);
        HWND favoriteNameLabel = createDialogControl(0,
                                                     L"STATIC",
                                                     tr(StringId::DisplayName, state->initialSettings.languageMode).data(),
                                                     WS_CHILD | WS_VISIBLE,
                                                     442,
                                                     232,
                                                     120,
                                                     18,
                                                     hwnd,
                                                     0);
        HWND favoriteLabelEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                     L"EDIT",
                                                     L"",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                     442,
                                                     254,
                                                     200,
                                                     24,
                                                     hwnd,
                                                     kFavoriteLabelEditId);
        HWND favoritePathLabel = createDialogControl(0,
                                                     L"STATIC",
                                                     tr(StringId::TargetPath, state->initialSettings.languageMode).data(),
                                                     WS_CHILD | WS_VISIBLE,
                                                     442,
                                                     292,
                                                     120,
                                                     18,
                                                     hwnd,
                                                     0);
        HWND favoritePathEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                    L"EDIT",
                                                    L"",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY,
                                                    442,
                                                    314,
                                                    200,
                                                    24,
                                                    hwnd,
                                                    kFavoritePathEditId);
        HWND favoriteUp = createDialogControl(0,
                                              L"BUTTON",
                                              tr(StringId::Up, state->initialSettings.languageMode).data(),
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                              442,
                                              360,
                                              58,
                                              28,
                                              hwnd,
                                              kFavoriteUpId);
        HWND favoriteDown = createDialogControl(0,
                                                L"BUTTON",
                                                tr(StringId::Down, state->initialSettings.languageMode).data(),
                                                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                508,
                                                360,
                                                62,
                                                28,
                                                hwnd,
                                                kFavoriteDownId);
        HWND favoriteRemove = createDialogControl(0,
                                                  L"BUTTON",
                                                  tr(StringId::Remove, state->initialSettings.languageMode).data(),
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                  578,
                                                  360,
                                                  64,
                                                  28,
                                                  hwnd,
                                                  kFavoriteRemoveId);
        HWND shortcutsGroup = createDialogControl(0,
                                                  L"BUTTON",
                                                  tr(StringId::Keyboard, state->initialSettings.languageMode).data(),
                                                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                  190,
                                                  78,
                                                  640,
                                                  534,
                                                  hwnd,
                                                  0);
        HWND shortcuts = createDialogControl(WS_EX_CLIENTEDGE,
                                             L"EDIT",
                                             shortcutHelpText(state->initialSettings.languageMode).c_str(),
                                             WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                             214,
                                             112,
                                             592,
                                             454,
                                             hwnd,
                                             0);
        HWND toolbarGroup = createDialogControl(0,
                                                L"BUTTON",
                                                tr(StringId::Toolbar, state->initialSettings.languageMode).data(),
                                                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                190,
                                                78,
                                                640,
                                                534,
                                                hwnd,
                                                0);
        HWND toolbarAvailableLabel = createDialogControl(0, L"STATIC", tr(StringId::AvailableActions, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE, 214, 122, 180, 20, hwnd, 0);
        HWND toolbarAvailableList = createDialogControl(WS_EX_CLIENTEDGE,
                                                        L"LISTBOX",
                                                        L"",
                                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                                                        214,
                                                        160,
                                                        200,
                                                        300,
                                                        hwnd,
                                                        kToolbarAvailableListId);
        HWND toolbarOrderLabel = createDialogControl(0, L"STATIC", tr(StringId::ToolbarOrder, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE, 586, 122, 180, 20, hwnd, 0);
        HWND toolbarList = createDialogControl(WS_EX_CLIENTEDGE,
                                               L"LISTBOX",
                                               L"",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                                               586,
                                               160,
                                               200,
                                               300,
                                               hwnd,
                                               kToolbarListId);
        HWND toolbarAdd = createDialogControl(0, L"BUTTON", tr(StringId::Add, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 448, 186, 104, 28, hwnd, kToolbarAddId);
        HWND toolbarRemove = createDialogControl(0, L"BUTTON", tr(StringId::Remove, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 448, 224, 104, 28, hwnd, kToolbarRemoveId);
        HWND toolbarUp = createDialogControl(0, L"BUTTON", tr(StringId::MoveUp, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 448, 292, 104, 28, hwnd, kToolbarUpId);
        HWND toolbarDown = createDialogControl(0, L"BUTTON", tr(StringId::MoveDown, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 448, 330, 104, 28, hwnd, kToolbarDownId);
        HWND contextGroup = createDialogControl(0,
                                                L"BUTTON",
                                                tr(StringId::ContextMenu, state->initialSettings.languageMode).data(),
                                                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                190,
                                                78,
                                                640,
                                                534,
                                                hwnd,
                                                0);
        HWND contextList = createDialogControl(WS_EX_CLIENTEDGE,
                                               L"LISTBOX",
                                               L"",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
                                               214,
                                               112,
                                               200,
                                               270,
                                               hwnd,
                                               kContextToolListId);
        HWND contextLabelText = createDialogControl(0, L"STATIC", tr(StringId::MenuLabel, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE, 438, 114, 120, 20, hwnd, 0);
        HWND contextLabelEdit = createDialogControl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 438, 136, 350, 24, hwnd, kContextToolLabelEditId);
        HWND contextExeText = createDialogControl(0, L"STATIC", tr(StringId::ProgramPath, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE, 438, 174, 120, 20, hwnd, 0);
        HWND contextExeEdit = createDialogControl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 438, 196, 266, 24, hwnd, kContextToolExeEditId);
        HWND contextBrowse = createDialogControl(0, L"BUTTON", tr(StringId::Browse, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 714, 194, 74, 28, hwnd, kContextToolBrowseId);
        HWND contextArgsText = createDialogControl(0, L"STATIC", tr(StringId::Arguments, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE, 438, 234, 120, 20, hwnd, 0);
        HWND contextArgsEdit = createDialogControl(WS_EX_CLIENTEDGE, L"EDIT", L"\"{path}\"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 438, 256, 350, 24, hwnd, kContextToolArgsEditId);
        HWND contextFiles = createDialogControl(0, L"BUTTON", tr(StringId::ShowForFiles, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 438, 296, 150, 22, hwnd, kContextToolFilesId);
        HWND contextFolders = createDialogControl(0, L"BUTTON", tr(StringId::ShowForFolders, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 600, 296, 170, 22, hwnd, kContextToolFoldersId);
        HWND contextVsCode = createDialogControl(0, L"BUTTON", L"VS Code", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 438, 334, 78, 28, hwnd, kContextToolVsCodeId);
        HWND contextAdd = createDialogControl(0, L"BUTTON", tr(StringId::Add, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 214, 404, 62, 28, hwnd, kContextToolAddId);
        HWND contextUpdate = createDialogControl(0, L"BUTTON", tr(StringId::Update, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 284, 404, 72, 28, hwnd, kContextToolUpdateId);
        HWND contextRemove = createDialogControl(0, L"BUTTON", tr(StringId::Remove, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 364, 404, 76, 28, hwnd, kContextToolRemoveId);
        HWND contextUp = createDialogControl(0, L"BUTTON", tr(StringId::Up, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 448, 404, 56, 28, hwnd, kContextToolUpId);
        HWND contextDown = createDialogControl(0, L"BUTTON", tr(StringId::Down, state->initialSettings.languageMode).data(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, 512, 404, 68, 28, hwnd, kContextToolDownId);
        HWND ok = createDialogControl(0,
                                      L"BUTTON",
                                      tr(StringId::Ok, state->initialSettings.languageMode).data(),
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                      706,
                                      640,
                                      76,
                                      28,
                                      hwnd,
                                      kOkId);
        HWND cancel = createDialogControl(0,
                                          L"BUTTON",
                                          tr(StringId::Cancel, state->initialSettings.languageMode).data(),
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          794,
                                          640,
                                          76,
                                          28,
                                          hwnd,
                                          kCancelId);
        if (!title || !subtitle || !categoryList || !appearanceGroup || !fontLabel || !fontEdit || !fontFamilyLabel || !fontFamilyCombo || !contextMenuFontLabel || !contextMenuFontEdit || !iconLabel || !iconEdit || !itemPaddingLabel || !itemPaddingEdit || !themeLabel || !themeCombo || !languageLabel || !languageCombo
            || !behaviorGroup || !showHiddenAndSystem || !windowGroup
            || !windowWidthLabel || !windowWidthEdit || !windowHeightLabel || !windowHeightEdit || !rememberWindowSize
            || !startupFolderLabel || !startupFolderEdit || !useCurrentFolder
            || !favoritesGroup || !favoritesLabel || !favoritesList
            || !favoriteNameLabel || !favoriteLabelEdit || !favoritePathLabel || !favoritePathEdit
            || !favoriteUp || !favoriteDown || !favoriteRemove || !shortcutsGroup || !shortcuts
            || !toolbarGroup || !toolbarAvailableLabel || !toolbarAvailableList || !toolbarOrderLabel || !toolbarList
            || !toolbarAdd || !toolbarRemove || !toolbarUp || !toolbarDown
            || !contextGroup || !contextList || !contextLabelText || !contextLabelEdit || !contextExeText || !contextExeEdit || !contextBrowse
            || !contextArgsText || !contextArgsEdit || !contextFiles || !contextFolders || !contextVsCode || !contextAdd || !contextUpdate
            || !contextRemove || !contextUp || !contextDown || !ok || !cancel) {
            return -1;
        }

        state->appearanceControls = {appearanceGroup, fontFamilyLabel, fontFamilyCombo, fontLabel, fontEdit, contextMenuFontLabel, contextMenuFontEdit, iconLabel, iconEdit, itemPaddingLabel, itemPaddingEdit, themeLabel, themeCombo, languageLabel, languageCombo};
        state->startupControls = {windowGroup, windowWidthLabel, windowWidthEdit, windowHeightLabel, windowHeightEdit, rememberWindowSize, startupFolderLabel, startupFolderEdit, useCurrentFolder};
        state->filesControls = {behaviorGroup, showHiddenAndSystem, favoritesGroup, favoritesLabel, favoritesList, favoriteNameLabel, favoriteLabelEdit, favoritePathLabel, favoritePathEdit, favoriteUp, favoriteDown, favoriteRemove};
        state->shortcutsControls = {shortcutsGroup, shortcuts};
        state->toolbarControls = {toolbarGroup, toolbarAvailableLabel, toolbarAvailableList, toolbarOrderLabel, toolbarList, toolbarAdd, toolbarRemove, toolbarUp, toolbarDown};
        state->contextMenuControls = {contextGroup, contextList, contextLabelText, contextLabelEdit, contextExeText, contextExeEdit, contextBrowse, contextArgsText, contextArgsEdit, contextFiles, contextFolders, contextVsCode, contextAdd, contextUpdate, contextRemove, contextUp, contextDown};
        state->panelControls = {appearanceGroup, windowGroup, behaviorGroup, favoritesGroup, shortcutsGroup, toolbarGroup, contextGroup};
        state->framedControls = {
            categoryList,
            fontFamilyCombo,
            fontEdit,
            contextMenuFontEdit,
            iconEdit,
            itemPaddingEdit,
            themeCombo,
            languageCombo,
            windowWidthEdit,
            windowHeightEdit,
            startupFolderEdit,
            favoritesList,
            favoriteLabelEdit,
            favoritePathEdit,
            shortcuts,
            toolbarAvailableList,
            toolbarList,
            contextList,
            contextLabelEdit,
            contextExeEdit,
            contextArgsEdit,
        };

        populateFavoritesList(hwnd, state->resultSettings, state->resultSettings.favorites.empty() ? -1 : 0);
        selectFavorite(hwnd, *state, state->resultSettings.favorites.empty() ? -1 : 0);
        refreshToolbarLists(hwnd, *state, state->resultSettings.toolbarCommands.empty() ? -1 : 0);
        populateContextToolList(hwnd, state->resultSettings, state->resultSettings.contextMenuTools.empty() ? -1 : 0);
        selectContextTool(hwnd, *state, state->resultSettings.contextMenuTools.empty() ? -1 : 0);
        showSettingsPage(*state, 0);
        layoutSettingsDialog(hwnd, *state);
        SetFocus(fontEdit);
        SendMessageW(fontEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_PAINT: {
        if (!state) {
            break;
        }
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(hwnd, &paint);
        paintSettingsDialog(hwnd, *state, dc);
        EndPaint(hwnd, &paint);
        return 0;
    }
    case WM_SIZE:
        if (state) {
            layoutSettingsDialog(hwnd, *state);
            return 0;
        }
        break;
    case WM_CTLCOLORDLG:
        if (state) {
            ensureThemeBrushes(*state);
            return reinterpret_cast<LRESULT>(state->backgroundBrush);
        }
        break;
    case WM_ERASEBKGND:
        if (state) {
            ensureThemeBrushes(*state);
            RECT rect{};
            GetClientRect(hwnd, &rect);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, state->backgroundBrush);
            return 1;
        }
        break;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        if (info) {
            const UINT dpi = dpiForWindow(hwnd);
            info->ptMinTrackSize.x = settingsDialogMinWidthForDpi(dpi);
            info->ptMinTrackSize.y = settingsDialogMinHeightForDpi(dpi);
            return 0;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        if (!state) {
            break;
        }
        ensureThemeBrushes(*state);
        const ThemeTokens tokens = themeTokens(state->resultSettings.themeMode);
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetTextColor(dc, colorRef(tokens.ink));
        SetBkMode(dc, OPAQUE);
        if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || isInputControl(control)) {
            SetBkColor(dc, colorRef(tokens.appInput));
            return reinterpret_cast<LRESULT>(state->inputBrush);
        }
        if (message == WM_CTLCOLORSTATIC && isTopHeaderControl(hwnd, control)) {
            SetBkMode(dc, TRANSPARENT);
            SetBkColor(dc, colorRef(tokens.app));
            return reinterpret_cast<LRESULT>(state->backgroundBrush);
        }
        if (message == WM_CTLCOLORSTATIC) {
            SetBkMode(dc, TRANSPARENT);
            SetBkColor(dc, colorRef(tokens.appOverlay));
            return reinterpret_cast<LRESULT>(state->panelBrush);
        }
        if (message == WM_CTLCOLORBTN) {
            SetBkColor(dc, colorRef(tokens.appOverlay));
            return reinterpret_cast<LRESULT>(state->panelBrush);
        }
        return reinterpret_cast<LRESULT>(state->backgroundBrush);
    }
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure && measure->CtlType == ODT_LISTBOX && isThemedListId(static_cast<int>(measure->CtlID))) {
            measure->itemHeight = sy(hwnd, 28);
            return TRUE;
        }
        if (measure && measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = sy(hwnd, 28);
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        if (!state) {
            break;
        }
        const auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (!draw) {
            break;
        }
        if (draw->CtlType == ODT_BUTTON && isOwnerDrawButtonId(static_cast<int>(draw->CtlID))) {
            drawOwnerButton(hwnd, *state, *draw);
            return TRUE;
        }
        if (draw->CtlType == ODT_LISTBOX && isThemedListId(static_cast<int>(draw->CtlID))) {
            drawOwnerListItem(*state, *draw);
            return TRUE;
        }
        if (draw->CtlType == ODT_COMBOBOX) {
            drawOwnerComboItem(*state, *draw);
            return TRUE;
        }
        if (draw->CtlType == ODT_STATIC) {
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
        if (!state) {
            break;
        }

        if (LOWORD(wParam) == kCategoryListId && HIWORD(wParam) == LBN_SELCHANGE) {
            const LRESULT index = SendMessageW(GetDlgItem(hwnd, kCategoryListId), LB_GETCURSEL, 0, 0);
            showSettingsPage(*state, index == LB_ERR ? 0 : static_cast<int>(index));
            layoutSettingsDialog(hwnd, *state);
            return 0;
        }

        if (LOWORD(wParam) == kFavoritesListId && HIWORD(wParam) == LBN_SELCHANGE) {
            selectFavorite(hwnd, *state, selectedFavoriteIndex(hwnd));
            return 0;
        }

        if (LOWORD(wParam) == kContextToolListId && HIWORD(wParam) == LBN_SELCHANGE) {
            selectContextTool(hwnd, *state, selectedContextToolIndex(hwnd));
            return 0;
        }

        if ((LOWORD(wParam) == kToolbarAvailableListId || LOWORD(wParam) == kToolbarListId)
            && HIWORD(wParam) == LBN_SELCHANGE) {
            updateToolbarButtons(hwnd, *state);
            return 0;
        }

        if (LOWORD(wParam) == kContextToolBrowseId) {
            std::wstring path;
            if (browseExecutable(hwnd, path, state->resultSettings.languageMode)) {
                SetWindowTextW(GetDlgItem(hwnd, kContextToolExeEditId), path.c_str());
            }
            return 0;
        }

        if (LOWORD(wParam) == kContextToolVsCodeId) {
            const std::wstring label(tr(StringId::OpenInVsCode, state->initialSettings.languageMode));
            setContextToolTemplate(hwnd, label.c_str(), L"Code.exe");
            return 0;
        }

        if (LOWORD(wParam) == kToolbarAddId) {
            const std::vector<ToolbarCommand> available = availableToolbarCommands();
            const int index = selectedAvailableToolbarIndex(hwnd);
            if (index >= 0 && index < static_cast<int>(available.size())
                && addToolbarCommand(state->resultSettings.toolbarCommands, available[static_cast<std::size_t>(index)])) {
                refreshToolbarLists(hwnd, *state, static_cast<int>(state->resultSettings.toolbarCommands.size()) - 1);
            } else {
                updateToolbarButtons(hwnd, *state);
            }
            return 0;
        }

        if (LOWORD(wParam) == kToolbarRemoveId) {
            const int index = selectedToolbarIndex(hwnd);
            if (index >= 0 && removeToolbarCommand(state->resultSettings.toolbarCommands, static_cast<std::size_t>(index))) {
                refreshToolbarLists(hwnd, *state, index);
            }
            return 0;
        }

        if (LOWORD(wParam) == kToolbarUpId || LOWORD(wParam) == kToolbarDownId) {
            const int index = selectedToolbarIndex(hwnd);
            const int direction = LOWORD(wParam) == kToolbarUpId ? -1 : 1;
            if (index >= 0 && moveToolbarCommand(state->resultSettings.toolbarCommands, static_cast<std::size_t>(index), direction)) {
                refreshToolbarLists(hwnd, *state, index + direction);
            }
            return 0;
        }

        if (LOWORD(wParam) == kFavoriteLabelEditId && HIWORD(wParam) == EN_CHANGE) {
            const int index = selectedFavoriteIndex(hwnd);
            if (index >= 0 && index < static_cast<int>(state->resultSettings.favorites.size())) {
                renameFavorite(state->resultSettings,
                               static_cast<std::size_t>(index),
                               getEditText(GetDlgItem(hwnd, kFavoriteLabelEditId)));
                populateFavoritesList(hwnd, state->resultSettings, index);
            }
            return 0;
        }

        if (LOWORD(wParam) == kFavoriteUpId) {
            syncSelectedFavoriteLabel(hwnd, *state);
            const int index = selectedFavoriteIndex(hwnd);
            if (index > 0 && moveFavorite(state->resultSettings, static_cast<std::size_t>(index), -1)) {
                const int nextIndex = index - 1;
                populateFavoritesList(hwnd, state->resultSettings, nextIndex);
                selectFavorite(hwnd, *state, nextIndex);
            }
            return 0;
        }

        if (LOWORD(wParam) == kFavoriteDownId) {
            syncSelectedFavoriteLabel(hwnd, *state);
            const int index = selectedFavoriteIndex(hwnd);
            if (index >= 0 && moveFavorite(state->resultSettings, static_cast<std::size_t>(index), 1)) {
                const int nextIndex = index + 1;
                populateFavoritesList(hwnd, state->resultSettings, nextIndex);
                selectFavorite(hwnd, *state, nextIndex);
            }
            return 0;
        }

        if (LOWORD(wParam) == kFavoriteRemoveId) {
            const int index = selectedFavoriteIndex(hwnd);
            if (index >= 0 && index < static_cast<int>(state->resultSettings.favorites.size())) {
                state->resultSettings.favorites.erase(state->resultSettings.favorites.begin() + index);
                int nextIndex = -1;
                if (!state->resultSettings.favorites.empty()) {
                    nextIndex = index < static_cast<int>(state->resultSettings.favorites.size())
                                    ? index
                                    : static_cast<int>(state->resultSettings.favorites.size()) - 1;
                }
                populateFavoritesList(hwnd, state->resultSettings, nextIndex);
                selectFavorite(hwnd, *state, nextIndex);
            }
            return 0;
        }

        if (LOWORD(wParam) == kUseCurrentFolderId) {
            if (!state->currentFolder.empty()) {
                SetWindowTextW(GetDlgItem(hwnd, kStartupFolderEditId), state->currentFolder.c_str());
            }
            return 0;
        }

        if (LOWORD(wParam) == kContextToolAddId) {
            ContextMenuTool tool = contextToolFromFields(hwnd);
            if (!tool.label.empty() && !tool.executablePath.empty()) {
                state->resultSettings.contextMenuTools.push_back(std::move(tool));
                const int nextIndex = static_cast<int>(state->resultSettings.contextMenuTools.size()) - 1;
                populateContextToolList(hwnd, state->resultSettings, nextIndex);
                selectContextTool(hwnd, *state, nextIndex);
            }
            return 0;
        }

        if (LOWORD(wParam) == kContextToolUpdateId) {
            const int index = selectedContextToolIndex(hwnd);
            if (index >= 0 && index < static_cast<int>(state->resultSettings.contextMenuTools.size())) {
                ContextMenuTool tool = contextToolFromFields(hwnd);
                if (!tool.label.empty() && !tool.executablePath.empty()) {
                    state->resultSettings.contextMenuTools[static_cast<std::size_t>(index)] = std::move(tool);
                    populateContextToolList(hwnd, state->resultSettings, index);
                    selectContextTool(hwnd, *state, index);
                }
            }
            return 0;
        }

        if (LOWORD(wParam) == kContextToolRemoveId) {
            const int index = selectedContextToolIndex(hwnd);
            if (index >= 0 && index < static_cast<int>(state->resultSettings.contextMenuTools.size())) {
                state->resultSettings.contextMenuTools.erase(state->resultSettings.contextMenuTools.begin() + index);
                int nextIndex = -1;
                if (!state->resultSettings.contextMenuTools.empty()) {
                    nextIndex = index < static_cast<int>(state->resultSettings.contextMenuTools.size())
                                    ? index
                                    : static_cast<int>(state->resultSettings.contextMenuTools.size()) - 1;
                }
                populateContextToolList(hwnd, state->resultSettings, nextIndex);
                selectContextTool(hwnd, *state, nextIndex);
            }
            return 0;
        }

        if (LOWORD(wParam) == kContextToolUpId || LOWORD(wParam) == kContextToolDownId) {
            syncSelectedContextTool(hwnd, *state);
            const int index = selectedContextToolIndex(hwnd);
            const int direction = LOWORD(wParam) == kContextToolUpId ? -1 : 1;
            const int nextIndex = index + direction;
            if (index >= 0
                && nextIndex >= 0
                && nextIndex < static_cast<int>(state->resultSettings.contextMenuTools.size())) {
                std::swap(state->resultSettings.contextMenuTools[static_cast<std::size_t>(index)],
                          state->resultSettings.contextMenuTools[static_cast<std::size_t>(nextIndex)]);
                populateContextToolList(hwnd, state->resultSettings, nextIndex);
                selectContextTool(hwnd, *state, nextIndex);
            }
            return 0;
        }

        if (LOWORD(wParam) == kOkId) {
            syncSelectedFavoriteLabel(hwnd, *state);
            syncSelectedContextTool(hwnd, *state);
            state->resultSettings.toolbarCommands = normalizeToolbarCommands(std::move(state->resultSettings.toolbarCommands));
            state->values.fontSizeText = getEditText(GetDlgItem(hwnd, kFontEditId));
            state->values.iconSizeText = getEditText(GetDlgItem(hwnd, kIconEditId));
            state->values.itemPaddingText = getEditText(GetDlgItem(hwnd, kItemPaddingEditId));
            state->values.themeModeText = comboSelectedText(GetDlgItem(hwnd, kThemeComboId));
            state->values.languageModeText = comboSelectedText(GetDlgItem(hwnd, kLanguageComboId));
            state->values.fontFamilyText = comboSelectedText(GetDlgItem(hwnd, kFontFamilyComboId));
            state->values.contextMenuFontSizeText = getEditText(GetDlgItem(hwnd, kContextMenuFontEditId));
            state->values.showHiddenAndSystemItems = SendMessageW(GetDlgItem(hwnd, kShowHiddenAndSystemId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            state->values.windowWidthText = getEditText(GetDlgItem(hwnd, kWindowWidthEditId));
            state->values.windowHeightText = getEditText(GetDlgItem(hwnd, kWindowHeightEditId));
            state->values.rememberWindowSize = SendMessageW(GetDlgItem(hwnd, kRememberWindowSizeId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            state->values.startupFolderText = getEditText(GetDlgItem(hwnd, kStartupFolderEditId));
            state->values.toolbarCommands = state->resultSettings.toolbarCommands;
            applySettingsDialogValues(state->values, state->resultSettings);
            state->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }

        if (LOWORD(wParam) == kCancelId) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (state) {
            deleteThemeBrushes(*state);
        }
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool registerSettingsDialogClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = settingsDialogProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kDialogClassName;

    if (RegisterClassExW(&wc) != 0) {
        return true;
    }

    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

} // namespace

bool settingsDialogSettingsEqual(const AppSettings& left, const AppSettings& right) {
    return settingsEqualInternal(left, right);
}

void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings) {
    float parsed = 0.0f;
    if (tryParseFloat(values.fontSizeText, parsed)) {
        settings.fontSize = parsed;
    }
    if (!values.fontFamilyText.empty()) {
        settings.fontFamily = values.fontFamilyText;
    }
    if (tryParseFloat(values.contextMenuFontSizeText, parsed)) {
        settings.contextMenuFontSize = parsed;
    }
    settings.showHiddenAndSystemItems = values.showHiddenAndSystemItems;
    if (tryParseFloat(values.iconSizeText, parsed)) {
        settings.iconSize = parsed;
    }
    if (tryParseFloat(values.itemPaddingText, parsed)) {
        settings.itemPadding = parsed;
    }
    if (tryParseFloat(values.windowWidthText, parsed)) {
        settings.windowWidth = static_cast<int>(parsed);
    }
    if (tryParseFloat(values.windowHeightText, parsed)) {
        settings.windowHeight = static_cast<int>(parsed);
    }
    settings.rememberWindowSize = values.rememberWindowSize;
    settings.startupFolder = values.startupFolderText;
    if (values.themeModeText == L"light"
        || values.themeModeText == tr(StringId::Light, LanguageMode::Chinese)
        || values.themeModeText == tr(StringId::Light, LanguageMode::English)) {
        settings.themeMode = ThemeMode::Light;
    } else if (values.themeModeText == L"dark"
        || values.themeModeText == tr(StringId::Dark, LanguageMode::Chinese)
        || values.themeModeText == tr(StringId::Dark, LanguageMode::English)) {
        settings.themeMode = ThemeMode::Dark;
    }
    if (values.languageModeText == L"zh-CN"
        || values.languageModeText == tr(StringId::Chinese, LanguageMode::Chinese)
        || values.languageModeText == tr(StringId::Chinese, LanguageMode::English)) {
        settings.languageMode = LanguageMode::Chinese;
    } else if (values.languageModeText == L"en-US"
        || values.languageModeText == tr(StringId::English, LanguageMode::Chinese)
        || values.languageModeText == tr(StringId::English, LanguageMode::English)) {
        settings.languageMode = LanguageMode::English;
    }
    if (!values.toolbarCommands.empty()) {
        settings.toolbarCommands = normalizeToolbarCommands(values.toolbarCommands);
    }

    clampSettings(settings);
}

float settingsDialogBodyFontSize() {
    return kSettingsBodyFontSize;
}

float settingsDialogTitleFontSize() {
    return kSettingsTitleFontSize;
}

DWORD settingsDialogWindowStyle() {
    return WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX;
}

DWORD settingsDialogListBoxStyle() {
    return LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT;
}

DWORD settingsDialogComboBoxStyle() {
    return CBS_OWNERDRAWVARIABLE | CBS_HASSTRINGS | WS_VSCROLL;
}

int settingsDialogComboDroppedHeight(int itemHeight, int visibleItems) {
    const int safeItemHeight = (std::max)(18, itemHeight);
    const int safeVisibleItems = (std::clamp)(visibleItems, 2, 16);
    return safeItemHeight * (safeVisibleItems + 1) + 4;
}

bool promptForSettings(HWND owner, AppSettings& settings, const std::wstring& currentFolder) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registerSettingsDialogClass(instance)) {
        return false;
    }

    SettingsDialogState state{};
    state.initialSettings = settings;
    state.resultSettings = settings;
    state.currentFolder = currentFolder;

    const DWORD exStyle = WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME;
    const DWORD style = settingsDialogWindowStyle();
    const UINT dpi = owner ? dpiForWindow(owner) : dpiForWindow(nullptr);
    HWND dialog = CreateWindowExW(exStyle,
                                  kDialogClassName,
                                  tr(StringId::Settings, settings.languageMode).data(),
                                  style,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  settingsDialogWidthForDpi(dpi),
                                  settingsDialogHeightForDpi(dpi),
                                  owner,
                                  nullptr,
                                  instance,
                                  &state);
    if (!dialog) {
        return false;
    }

    applyNativeTitleBarTheme(dialog, settings.themeMode);
    centerDialog(dialog, owner);

    if (owner) {
        EnableWindow(owner, FALSE);
    }

    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);

    bool sawQuitMessage = false;
    bool messageLoopError = false;
    WPARAM quitCode = 0;

    MSG message{};
    while (IsWindow(dialog)) {
        const int getMessageResult = GetMessageW(&message, nullptr, 0, 0);
        if (getMessageResult == -1) {
            messageLoopError = true;
            if (IsWindow(dialog)) {
                DestroyWindow(dialog);
            }
            break;
        }

        if (getMessageResult == 0) {
            sawQuitMessage = true;
            quitCode = message.wParam;
            if (IsWindow(dialog)) {
                DestroyWindow(dialog);
            }
            break;
        }

        if (IsWindow(dialog) && IsDialogMessageW(dialog, &message)) {
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (owner) {
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
        SetFocus(owner);
    }

    if (sawQuitMessage) {
        PostQuitMessage(static_cast<int>(quitCode));
        return false;
    }

    if (messageLoopError || !state.accepted || settingsDialogSettingsEqual(state.initialSettings, state.resultSettings)) {
        return false;
    }

    settings = state.resultSettings;
    return true;
}

} // namespace finderx::ui
