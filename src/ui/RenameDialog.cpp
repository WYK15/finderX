#include "ui/RenameDialog.h"

namespace finderx::ui {

namespace {

struct RenameDialogState {
    std::wstring currentName;
    std::wstring result;
    bool accepted = false;
};

constexpr wchar_t kDialogClassName[] = L"FinderXRenameDialog";
constexpr int kEditId = 101;
constexpr int kOkId = IDOK;
constexpr int kCancelId = IDCANCEL;
constexpr int kDialogWidth = 460;
constexpr int kDialogHeight = 170;

std::wstring upperAscii(std::wstring value) {
    for (wchar_t& ch : value) {
        if (ch >= L'a' && ch <= L'z') {
            ch = static_cast<wchar_t>(ch - L'a' + L'A');
        }
    }
    return value;
}

bool isReservedDosDeviceName(const std::wstring& name) {
    const std::size_t extensionOffset = name.find(L'.');
    const std::wstring baseName = upperAscii(name.substr(0, extensionOffset));

    if (baseName == L"CON" || baseName == L"PRN" || baseName == L"AUX" || baseName == L"NUL") {
        return true;
    }

    if (baseName.size() == 4 && (baseName.starts_with(L"COM") || baseName.starts_with(L"LPT"))) {
        return baseName[3] >= L'1' && baseName[3] <= L'9';
    }

    return false;
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

LRESULT CALLBACK renameDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<RenameDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        state = static_cast<RenameDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    switch (message) {
    case WM_CREATE: {
        if (!state) {
            return -1;
        }

        HWND label = createDialogControl(0,
                                         L"STATIC",
                                         L"Name:",
                                         WS_CHILD | WS_VISIBLE,
                                         14,
                                         18,
                                         48,
                                         22,
                                         hwnd,
                                         0);
        HWND edit = createDialogControl(WS_EX_CLIENTEDGE,
                                        L"EDIT",
                                        state->currentName.c_str(),
                                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                        64,
                                        14,
                                        364,
                                        24,
                                        hwnd,
                                        kEditId);
        HWND ok = createDialogControl(0,
                                      L"BUTTON",
                                      L"OK",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                      252,
                                      82,
                                      82,
                                      30,
                                      hwnd,
                                      kOkId);
        HWND cancel = createDialogControl(0,
                                          L"BUTTON",
                                          L"Cancel",
                                          WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                          346,
                                          82,
                                          82,
                                          30,
                                          hwnd,
                                          kCancelId);
        if (!label || !edit || !ok || !cancel) {
            return -1;
        }

        SetFocus(edit);
        SendMessageW(edit, EM_SETSEL, 0, -1);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kOkId) {
            state->result = getEditText(GetDlgItem(hwnd, kEditId));
            if (!isValidRenameName(state->result)) {
                MessageBoxW(hwnd, L"Enter a valid file name.", L"Rename", MB_ICONWARNING | MB_OK);
                SetFocus(GetDlgItem(hwnd, kEditId));
                return 0;
            }

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

bool registerRenameDialogClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = renameDialogProc;
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

bool isValidRenameName(const std::wstring& name) {
    if (name.empty()) {
        return false;
    }

    if (name == L"." || name == L"..") {
        return false;
    }

    if (name.back() == L' ' || name.back() == L'.') {
        return false;
    }

    if (name.find_first_of(LR"(\/:*?"<>|)") != std::wstring::npos) {
        return false;
    }

    for (wchar_t ch : name) {
        if (ch < 0x20) {
            return false;
        }
    }

    return !isReservedDosDeviceName(name);
}

bool promptForRename(HWND owner, const std::wstring& currentName, std::wstring& newName) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registerRenameDialogClass(instance)) {
        return false;
    }

    RenameDialogState state{currentName};

    const DWORD exStyle = WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME;
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    HWND dialog = CreateWindowExW(exStyle,
                                  kDialogClassName,
                                  L"Rename",
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

    if (messageLoopError) {
        return false;
    }

    if (!state.accepted || state.result == state.currentName) {
        return false;
    }

    newName = state.result;
    return true;
}

} // namespace finderx::ui
