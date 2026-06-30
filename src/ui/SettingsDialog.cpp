#include "ui/SettingsDialog.h"

#include <cmath>
#include <cwchar>

namespace finderx::ui {

namespace {

struct SettingsDialogState {
    SettingsDialogValues values;
    AppSettings initialSettings;
    AppSettings resultSettings;
    bool accepted = false;
};

constexpr wchar_t kDialogClassName[] = L"FinderXSettingsDialog";
constexpr int kFontEditId = 101;
constexpr int kIconEditId = 102;
constexpr int kOkId = IDOK;
constexpr int kCancelId = IDCANCEL;
constexpr int kDialogWidth = 328;
constexpr int kDialogHeight = 158;

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
    if (left.fontSize != right.fontSize || left.iconSize != right.iconSize
        || left.sortColumn != right.sortColumn || left.sortDirection != right.sortDirection
        || left.favorites.size() != right.favorites.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.favorites.size(); ++index) {
        if (left.favorites[index].label != right.favorites[index].label
            || left.favorites[index].path != right.favorites[index].path) {
            return false;
        }
    }
    return true;
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

        HWND fontLabel = createDialogControl(0,
                                             L"STATIC",
                                             L"Font size:",
                                             WS_CHILD | WS_VISIBLE,
                                             16,
                                             18,
                                             88,
                                             22,
                                             hwnd,
                                             0);
        HWND fontEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                            L"EDIT",
                                            formatSettingValue(state->initialSettings.fontSize).c_str(),
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                            112,
                                            14,
                                            184,
                                            24,
                                            hwnd,
                                            kFontEditId);
        HWND iconLabel = createDialogControl(0,
                                             L"STATIC",
                                             L"Icon size:",
                                             WS_CHILD | WS_VISIBLE,
                                             16,
                                             52,
                                             88,
                                             22,
                                             hwnd,
                                             0);
        HWND iconEdit = createDialogControl(WS_EX_CLIENTEDGE,
                                            L"EDIT",
                                            formatSettingValue(state->initialSettings.iconSize).c_str(),
                                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                            112,
                                            48,
                                            184,
                                            24,
                                            hwnd,
                                            kIconEditId);
        HWND ok = createDialogControl(0,
                                      L"BUTTON",
                                      L"OK",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                      136,
                                      88,
                                      76,
                                      28,
                                      hwnd,
                                      kOkId);
        HWND cancel = createDialogControl(0,
                                          L"BUTTON",
                                          L"Cancel",
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          220,
                                          88,
                                          76,
                                          28,
                                          hwnd,
                                          kCancelId);
        if (!fontLabel || !fontEdit || !iconLabel || !iconEdit || !ok || !cancel) {
            return -1;
        }

        SetFocus(fontEdit);
        SendMessageW(fontEdit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kOkId) {
            state->values.fontSizeText = getEditText(GetDlgItem(hwnd, kFontEditId));
            state->values.iconSizeText = getEditText(GetDlgItem(hwnd, kIconEditId));
            state->resultSettings = state->initialSettings;
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
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
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
    if (tryParseFloat(values.iconSizeText, parsed)) {
        settings.iconSize = parsed;
    }

    clampSettings(settings);
}

bool promptForSettings(HWND owner, AppSettings& settings) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registerSettingsDialogClass(instance)) {
        return false;
    }

    SettingsDialogState state{};
    state.initialSettings = settings;
    state.resultSettings = settings;

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
