#include "ui/SettingsDialog.h"
#include "ui/NativeTitleBar.h"
#include "ui/ThemeTokens.h"

#include <algorithm>
#include <cmath>
#include <cwchar>

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
    std::vector<HWND> appearanceControls;
    std::vector<HWND> startupControls;
    std::vector<HWND> filesControls;
    std::vector<HWND> shortcutsControls;
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
constexpr int kContextToolZedId = 314;
constexpr int kOkId = IDOK;
constexpr int kCancelId = IDCANCEL;
constexpr int kDialogWidth = 900;
constexpr int kDialogHeight = 720;

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
        || id == kIconEditId
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
        || id == kThemeComboId
        || id == kFontFamilyComboId;
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
    showControls(state.contextMenuControls, page == 4);
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
}

std::wstring shortcutHelpText() {
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

void setControlFont(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
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
    HWND control = CreateWindowExW(exStyle,
                                   className,
                                   text,
                                   style,
                                   x,
                                   y,
                                   width,
                                   height,
                                   parent,
                                   id == 0 ? nullptr : reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   GetModuleHandleW(nullptr),
                                   nullptr);
    if (control) {
        setControlFont(control);
    }
    return control;
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

bool settingsEqual(const AppSettings& left, const AppSettings& right) {
    if (left.fontSize != right.fontSize || left.fontFamily != right.fontFamily || left.iconSize != right.iconSize
        || left.windowWidth != right.windowWidth || left.windowHeight != right.windowHeight
        || left.rememberWindowSize != right.rememberWindowSize
        || left.startupFolder != right.startupFolder
        || left.themeMode != right.themeMode || left.showHiddenAndSystemItems != right.showHiddenAndSystemItems
        || left.sortColumn != right.sortColumn || left.sortDirection != right.sortDirection
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

void populateFontFamilyCombo(HWND combo, const std::wstring& selectedFontFamily) {
    static constexpr const wchar_t* kFontFamilies[] = {
        L"Segoe UI",
        L"Microsoft YaHei UI",
        L"Microsoft YaHei",
        L"SimSun",
        L"Consolas",
        L"Arial",
    };

    for (const wchar_t* fontFamily : kFontFamilies) {
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontFamily));
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

bool browseExecutable(HWND hwnd, std::wstring& path) {
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
    openFile.lpstrTitle = L"Choose Program";
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

void centerDialog(HWND hwnd, HWND owner) {
    RECT anchor{};
    if (owner && GetWindowRect(owner, &anchor)) {
        const int x = anchor.left + ((anchor.right - anchor.left) - kDialogWidth) / 2;
        const int y = anchor.top + ((anchor.bottom - anchor.top) - kDialogHeight) / 2;
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
        return;
    }

    RECT workArea{};
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0)) {
        workArea = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    }

    const int x = workArea.left + ((workArea.right - workArea.left) - kDialogWidth) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - kDialogHeight) / 2;
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

        HWND title = createDialogControl(0,
                                         L"STATIC",
                                         L"FinderX Settings",
                                         WS_CHILD | WS_VISIBLE,
                                         24,
                                         18,
                                         320,
                                         24,
                                         hwnd,
                                         0);
        HWND subtitle = createDialogControl(0,
                                            L"STATIC",
                                            L"Configure appearance, browsing behavior, window startup, favorites, and shortcuts.",
                                            WS_CHILD | WS_VISIBLE,
                                            24,
                                            44,
                                            760,
                                            22,
                                            hwnd,
                                            0);
        HWND categoryList = createDialogControl(WS_EX_CLIENTEDGE,
                                                L"LISTBOX",
                                                L"",
                                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY,
                                                24,
                                                78,
                                                140,
                                                508,
                                                hwnd,
                                                kCategoryListId);
        if (categoryList) {
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Appearance"));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Startup"));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Files"));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Shortcuts"));
            SendMessageW(categoryList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Context Menu"));
            SendMessageW(categoryList, LB_SETCURSEL, 0, 0);
        }
        HWND appearanceGroup = createDialogControl(0,
                                                   L"BUTTON",
                                                   L"Appearance",
                                                   WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                   190,
                                                   78,
                                                   640,
                                                   176,
                                                   hwnd,
                                                   0);
        HWND fontFamilyLabel = createDialogControl(0,
                                                   L"STATIC",
                                                   L"Font family",
                                                   WS_CHILD | WS_VISIBLE,
                                                   214,
                                                   110,
                                                   110,
                                                   22,
                                                   hwnd,
                                                   0);
        HWND fontFamilyCombo = createDialogControl(0,
                                                   L"COMBOBOX",
                                                   L"",
                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                                   342,
                                                   106,
                                                   220,
                                                   160,
                                                   hwnd,
                                                   kFontFamilyComboId);
        if (fontFamilyCombo) {
            populateFontFamilyCombo(fontFamilyCombo, state->initialSettings.fontFamily);
        }
        HWND fontLabel = createDialogControl(0,
                                             L"STATIC",
                                             L"Text size",
                                             WS_CHILD | WS_VISIBLE,
                                             214,
                                             146,
                                             110,
                                             22,
                                             hwnd,
                                             0);
        HWND fontEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                            L"EDIT",
                                            formatSettingValue(state->initialSettings.fontSize).c_str(),
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                            342,
                                            142,
                                            164,
                                            24,
                                            hwnd,
                                            kFontEditId);
        HWND iconLabel = createDialogControl(0,
                                             L"STATIC",
                                             L"Icon size",
                                             WS_CHILD | WS_VISIBLE,
                                             214,
                                             182,
                                             110,
                                             22,
                                             hwnd,
                                             0);
        HWND iconEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                            L"EDIT",
                                            formatSettingValue(state->initialSettings.iconSize).c_str(),
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                            342,
                                            178,
                                            164,
                                            24,
                                            hwnd,
                                            kIconEditId);
        HWND themeLabel = createDialogControl(0,
                                              L"STATIC",
                                              L"Theme",
                                              WS_CHILD | WS_VISIBLE,
                                              214,
                                              218,
                                              110,
                                              22,
                                              hwnd,
                                              0);
        HWND themeCombo = createDialogControl(0,
                                              L"COMBOBOX",
                                              L"",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
                                              342,
                                              214,
                                              164,
                                              120,
                                              hwnd,
                                              kThemeComboId);
        if (themeCombo) {
            SendMessageW(themeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"light"));
            SendMessageW(themeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"dark"));
            SendMessageW(themeCombo, CB_SETCURSEL, state->initialSettings.themeMode == ThemeMode::Light ? 0 : 1, 0);
        }
        HWND behaviorGroup = createDialogControl(0,
                                                 L"BUTTON",
                                                 L"File Browsing",
                                                 WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                 190,
                                                 78,
                                                 640,
                                                 78,
                                                 hwnd,
                                                 0);
        HWND showHiddenAndSystem = createDialogControl(0,
                                                       L"BUTTON",
                                                       L"Show hidden and system items",
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
                                               L"Window",
                                               WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                               190,
                                               176,
                                               640,
                                               120,
                                               hwnd,
                                               0);
        HWND windowWidthLabel = createDialogControl(0,
                                                    L"STATIC",
                                                    L"Width",
                                                    WS_CHILD | WS_VISIBLE,
                                                    214,
                                                    208,
                                                    48,
                                                    22,
                                                    hwnd,
                                                    0);
        HWND windowWidthEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                   L"EDIT",
                                                   std::to_wstring(state->initialSettings.windowWidth).c_str(),
                                                   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                   268,
                                                   204,
                                                   74,
                                                   24,
                                                   hwnd,
                                                   kWindowWidthEditId);
        HWND windowHeightLabel = createDialogControl(0,
                                                     L"STATIC",
                                                     L"Height",
                                                     WS_CHILD | WS_VISIBLE,
                                                     372,
                                                     208,
                                                     52,
                                                     22,
                                                     hwnd,
                                                     0);
        HWND windowHeightEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                    L"EDIT",
                                                    std::to_wstring(state->initialSettings.windowHeight).c_str(),
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                    434,
                                                    204,
                                                    74,
                                                    24,
                                                    hwnd,
                                                    kWindowHeightEditId);
        HWND rememberWindowSize = createDialogControl(0,
                                                      L"BUTTON",
                                                      L"Remember last window size",
                                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                                      214,
                                                      240,
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
                                                      L"Startup folder",
                                                      WS_CHILD | WS_VISIBLE,
                                                      214,
                                                      266,
                                                      110,
                                                      22,
                                                      hwnd,
                                                      0);
        HWND startupFolderEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                     L"EDIT",
                                                     state->initialSettings.startupFolder.c_str(),
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                     324,
                                                     262,
                                                     330,
                                                     24,
                                                     hwnd,
                                                     kStartupFolderEditId);
        HWND useCurrentFolder = createDialogControl(0,
                                                    L"BUTTON",
                                                    L"Use current",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                    664,
                                                    260,
                                                    84,
                                                    28,
                                                    hwnd,
                                                    kUseCurrentFolderId);
        if (useCurrentFolder && state->currentFolder.empty()) {
            EnableWindow(useCurrentFolder, FALSE);
        }
        HWND favoritesGroup = createDialogControl(0,
                                                  L"BUTTON",
                                                  L"Favorites",
                                                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                  190,
                                                  318,
                                                  640,
                                                  268,
                                                  hwnd,
                                                  0);
        HWND favoritesLabel = createDialogControl(0,
                                                  L"STATIC",
                                                  L"Order and rename sidebar favorites.",
                                                  WS_CHILD | WS_VISIBLE,
                                                  214,
                                                  348,
                                                  260,
                                                  18,
                                                  hwnd,
                                                  0);
        HWND favoritesList = createDialogControl(WS_EX_CLIENTEDGE,
                                                 L"LISTBOX",
                                                 L"",
                                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
                                                 214,
                                                 372,
                                                 180,
                                                 174,
                                                 hwnd,
                                                 kFavoritesListId);
        HWND favoriteNameLabel = createDialogControl(0,
                                                     L"STATIC",
                                                     L"Display name",
                                                     WS_CHILD | WS_VISIBLE,
                                                     442,
                                                     374,
                                                     120,
                                                     18,
                                                     hwnd,
                                                     0);
        HWND favoriteLabelEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                     L"EDIT",
                                                     L"",
                                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                     442,
                                                     396,
                                                     200,
                                                     24,
                                                     hwnd,
                                                     kFavoriteLabelEditId);
        HWND favoritePathLabel = createDialogControl(0,
                                                     L"STATIC",
                                                     L"Target path",
                                                     WS_CHILD | WS_VISIBLE,
                                                     442,
                                                     434,
                                                     120,
                                                     18,
                                                     hwnd,
                                                     0);
        HWND favoritePathEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                                    L"EDIT",
                                                    L"",
                                                    WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_READONLY,
                                                    442,
                                                    456,
                                                    200,
                                                    24,
                                                    hwnd,
                                                    kFavoritePathEditId);
        HWND favoriteUp = createDialogControl(0,
                                              L"BUTTON",
                                              L"Up",
                                              WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                              442,
                                              502,
                                              58,
                                              28,
                                              hwnd,
                                              kFavoriteUpId);
        HWND favoriteDown = createDialogControl(0,
                                                L"BUTTON",
                                                L"Down",
                                                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                508,
                                                502,
                                                62,
                                                28,
                                                hwnd,
                                                kFavoriteDownId);
        HWND favoriteRemove = createDialogControl(0,
                                                  L"BUTTON",
                                                  L"Remove",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                  578,
                                                  502,
                                                  64,
                                                  28,
                                                  hwnd,
                                                  kFavoriteRemoveId);
        HWND shortcutsGroup = createDialogControl(0,
                                                  L"BUTTON",
                                                  L"Keyboard",
                                                  WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                  190,
                                                  318,
                                                  640,
                                                  268,
                                                  hwnd,
                                                  0);
        HWND shortcuts = createDialogControl(WS_EX_CLIENTEDGE,
                                             L"EDIT",
                                             shortcutHelpText().c_str(),
                                             WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                             214,
                                             348,
                                             592,
                                             198,
                                             hwnd,
                                             0);
        HWND contextGroup = createDialogControl(0,
                                                L"BUTTON",
                                                L"Context Menu",
                                                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                                190,
                                                78,
                                                640,
                                                508,
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
        HWND contextLabelText = createDialogControl(0, L"STATIC", L"Menu label", WS_CHILD | WS_VISIBLE, 438, 114, 120, 20, hwnd, 0);
        HWND contextLabelEdit = createDialogControl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 438, 136, 350, 24, hwnd, kContextToolLabelEditId);
        HWND contextExeText = createDialogControl(0, L"STATIC", L"Program path", WS_CHILD | WS_VISIBLE, 438, 174, 120, 20, hwnd, 0);
        HWND contextExeEdit = createDialogControl(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 438, 196, 266, 24, hwnd, kContextToolExeEditId);
        HWND contextBrowse = createDialogControl(0, L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 714, 194, 74, 28, hwnd, kContextToolBrowseId);
        HWND contextArgsText = createDialogControl(0, L"STATIC", L"Arguments", WS_CHILD | WS_VISIBLE, 438, 234, 120, 20, hwnd, 0);
        HWND contextArgsEdit = createDialogControl(WS_EX_CLIENTEDGE, L"EDIT", L"\"{path}\"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 438, 256, 350, 24, hwnd, kContextToolArgsEditId);
        HWND contextFiles = createDialogControl(0, L"BUTTON", L"Show for files", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 438, 296, 150, 22, hwnd, kContextToolFilesId);
        HWND contextFolders = createDialogControl(0, L"BUTTON", L"Show for folders", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 600, 296, 170, 22, hwnd, kContextToolFoldersId);
        HWND contextVsCode = createDialogControl(0, L"BUTTON", L"VS Code", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 438, 334, 78, 28, hwnd, kContextToolVsCodeId);
        HWND contextZed = createDialogControl(0, L"BUTTON", L"Zed", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 524, 334, 62, 28, hwnd, kContextToolZedId);
        HWND contextAdd = createDialogControl(0, L"BUTTON", L"Add", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 214, 404, 62, 28, hwnd, kContextToolAddId);
        HWND contextUpdate = createDialogControl(0, L"BUTTON", L"Update", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 284, 404, 72, 28, hwnd, kContextToolUpdateId);
        HWND contextRemove = createDialogControl(0, L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 364, 404, 76, 28, hwnd, kContextToolRemoveId);
        HWND contextUp = createDialogControl(0, L"BUTTON", L"Up", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 448, 404, 56, 28, hwnd, kContextToolUpId);
        HWND contextDown = createDialogControl(0, L"BUTTON", L"Down", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 512, 404, 68, 28, hwnd, kContextToolDownId);
        HWND ok = createDialogControl(0,
                                      L"BUTTON",
                                      L"OK",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                      706,
                                      640,
                                      76,
                                      28,
                                      hwnd,
                                      kOkId);
        HWND cancel = createDialogControl(0,
                                          L"BUTTON",
                                          L"Cancel",
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          794,
                                          640,
                                          76,
                                          28,
                                          hwnd,
                                          kCancelId);
        if (!title || !subtitle || !categoryList || !appearanceGroup || !fontLabel || !fontEdit || !fontFamilyLabel || !fontFamilyCombo || !iconLabel || !iconEdit || !themeLabel || !themeCombo
            || !behaviorGroup || !showHiddenAndSystem || !windowGroup
            || !windowWidthLabel || !windowWidthEdit || !windowHeightLabel || !windowHeightEdit || !rememberWindowSize
            || !startupFolderLabel || !startupFolderEdit || !useCurrentFolder
            || !favoritesGroup || !favoritesLabel || !favoritesList
            || !favoriteNameLabel || !favoriteLabelEdit || !favoritePathLabel || !favoritePathEdit
            || !favoriteUp || !favoriteDown || !favoriteRemove || !shortcutsGroup || !shortcuts
            || !contextGroup || !contextList || !contextLabelText || !contextLabelEdit || !contextExeText || !contextExeEdit || !contextBrowse
            || !contextArgsText || !contextArgsEdit || !contextFiles || !contextFolders || !contextVsCode || !contextZed || !contextAdd || !contextUpdate
            || !contextRemove || !contextUp || !contextDown || !ok || !cancel) {
            return -1;
        }

        state->appearanceControls = {appearanceGroup, fontFamilyLabel, fontFamilyCombo, fontLabel, fontEdit, iconLabel, iconEdit, themeLabel, themeCombo};
        state->startupControls = {windowGroup, windowWidthLabel, windowWidthEdit, windowHeightLabel, windowHeightEdit, rememberWindowSize, startupFolderLabel, startupFolderEdit, useCurrentFolder};
        state->filesControls = {behaviorGroup, showHiddenAndSystem, favoritesGroup, favoritesLabel, favoritesList, favoriteNameLabel, favoriteLabelEdit, favoritePathLabel, favoritePathEdit, favoriteUp, favoriteDown, favoriteRemove};
        state->shortcutsControls = {shortcutsGroup, shortcuts};
        state->contextMenuControls = {contextGroup, contextList, contextLabelText, contextLabelEdit, contextExeText, contextExeEdit, contextBrowse, contextArgsText, contextArgsEdit, contextFiles, contextFolders, contextVsCode, contextZed, contextAdd, contextUpdate, contextRemove, contextUp, contextDown};

        populateFavoritesList(hwnd, state->resultSettings, state->resultSettings.favorites.empty() ? -1 : 0);
        selectFavorite(hwnd, *state, state->resultSettings.favorites.empty() ? -1 : 0);
        populateContextToolList(hwnd, state->resultSettings, state->resultSettings.contextMenuTools.empty() ? -1 : 0);
        selectContextTool(hwnd, *state, state->resultSettings.contextMenuTools.empty() ? -1 : 0);
        showSettingsPage(*state, 0);
        SetFocus(fontEdit);
        SendMessageW(fontEdit, EM_SETSEL, 0, -1);
        return 0;
    }
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
        SetBkMode(dc, TRANSPARENT);
        if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || isInputControl(control)) {
            SetBkMode(dc, OPAQUE);
            SetBkColor(dc, colorRef(tokens.appInput));
            return reinterpret_cast<LRESULT>(state->inputBrush);
        }
        if (message == WM_CTLCOLORBTN) {
            return reinterpret_cast<LRESULT>(state->panelBrush);
        }
        return reinterpret_cast<LRESULT>(state->backgroundBrush);
    }
    case WM_COMMAND:
        if (!state) {
            break;
        }

        if (LOWORD(wParam) == kCategoryListId && HIWORD(wParam) == LBN_SELCHANGE) {
            const LRESULT index = SendMessageW(GetDlgItem(hwnd, kCategoryListId), LB_GETCURSEL, 0, 0);
            showSettingsPage(*state, index == LB_ERR ? 0 : static_cast<int>(index));
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

        if (LOWORD(wParam) == kContextToolBrowseId) {
            std::wstring path;
            if (browseExecutable(hwnd, path)) {
                SetWindowTextW(GetDlgItem(hwnd, kContextToolExeEditId), path.c_str());
            }
            return 0;
        }

        if (LOWORD(wParam) == kContextToolVsCodeId) {
            setContextToolTemplate(hwnd, L"Open in VS Code", L"Code.exe");
            return 0;
        }

        if (LOWORD(wParam) == kContextToolZedId) {
            setContextToolTemplate(hwnd, L"Open in Zed", L"zed.exe");
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
            state->values.fontSizeText = getEditText(GetDlgItem(hwnd, kFontEditId));
            state->values.iconSizeText = getEditText(GetDlgItem(hwnd, kIconEditId));
            state->values.themeModeText = comboSelectedText(GetDlgItem(hwnd, kThemeComboId));
            state->values.fontFamilyText = comboSelectedText(GetDlgItem(hwnd, kFontFamilyComboId));
            state->values.showHiddenAndSystemItems = SendMessageW(GetDlgItem(hwnd, kShowHiddenAndSystemId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            state->values.windowWidthText = getEditText(GetDlgItem(hwnd, kWindowWidthEditId));
            state->values.windowHeightText = getEditText(GetDlgItem(hwnd, kWindowHeightEditId));
            state->values.rememberWindowSize = SendMessageW(GetDlgItem(hwnd, kRememberWindowSizeId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            state->values.startupFolderText = getEditText(GetDlgItem(hwnd, kStartupFolderEditId));
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

void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings) {
    float parsed = 0.0f;
    if (tryParseFloat(values.fontSizeText, parsed)) {
        settings.fontSize = parsed;
    }
    if (!values.fontFamilyText.empty()) {
        settings.fontFamily = values.fontFamilyText;
    }
    settings.showHiddenAndSystemItems = values.showHiddenAndSystemItems;
    if (tryParseFloat(values.iconSizeText, parsed)) {
        settings.iconSize = parsed;
    }
    if (tryParseFloat(values.windowWidthText, parsed)) {
        settings.windowWidth = static_cast<int>(parsed);
    }
    if (tryParseFloat(values.windowHeightText, parsed)) {
        settings.windowHeight = static_cast<int>(parsed);
    }
    settings.rememberWindowSize = values.rememberWindowSize;
    settings.startupFolder = values.startupFolderText;
    if (values.themeModeText == L"light") {
        settings.themeMode = ThemeMode::Light;
    } else if (values.themeModeText == L"dark") {
        settings.themeMode = ThemeMode::Dark;
    }

    clampSettings(settings);
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
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    HWND dialog = CreateWindowExW(exStyle,
                                  kDialogClassName,
                                  L"Settings",
                                  style,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  kDialogWidth,
                                  kDialogHeight,
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

    if (messageLoopError || !state.accepted || settingsEqual(state.initialSettings, state.resultSettings)) {
        return false;
    }

    settings = state.resultSettings;
    return true;
}

} // namespace finderx::ui
