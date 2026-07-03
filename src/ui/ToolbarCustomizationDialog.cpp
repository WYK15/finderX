#include "ui/ToolbarCustomizationDialog.h"

#include "ui/ToolbarCustomization.h"

namespace finderx::ui {
namespace {

constexpr wchar_t kDialogClassName[] = L"FinderXToolbarCustomizationDialog";
constexpr int kAvailableListId = 101;
constexpr int kToolbarListId = 102;
constexpr int kAddId = 103;
constexpr int kRemoveId = 104;
constexpr int kUpId = 105;
constexpr int kDownId = 106;
constexpr int kOkId = IDOK;
constexpr int kCancelId = IDCANCEL;
constexpr int kDialogWidth = 560;
constexpr int kDialogHeight = 360;

struct ToolbarDialogState {
    AppSettings initialSettings;
    AppSettings resultSettings;
    bool accepted = false;
};

void setControlFont(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

HWND createControl(const wchar_t* className, const wchar_t* text, DWORD style, int x, int y, int width, int height, HWND parent, int id, DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle,
                                   className,
                                   text,
                                   WS_CHILD | WS_VISIBLE | style,
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

int selectedListIndex(HWND hwnd, int id) {
    HWND list = GetDlgItem(hwnd, id);
    if (!list) {
        return -1;
    }
    const LRESULT selected = SendMessageW(list, LB_GETCURSEL, 0, 0);
    return selected == LB_ERR ? -1 : static_cast<int>(selected);
}

void populateList(HWND list, const std::vector<ToolbarCommand>& commands) {
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const ToolbarCommand command : commands) {
        const std::wstring label = toolbarCommandLabel(command);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
}

void refreshLists(HWND hwnd, const ToolbarDialogState& state, int selectedToolbarIndex = -1) {
    HWND available = GetDlgItem(hwnd, kAvailableListId);
    HWND toolbar = GetDlgItem(hwnd, kToolbarListId);
    if (!available || !toolbar) {
        return;
    }

    populateList(available, availableToolbarCommands());
    populateList(toolbar, state.resultSettings.toolbarCommands);
    if (selectedToolbarIndex >= 0 && selectedToolbarIndex < static_cast<int>(state.resultSettings.toolbarCommands.size())) {
        SendMessageW(toolbar, LB_SETCURSEL, static_cast<WPARAM>(selectedToolbarIndex), 0);
    }
}

void centerDialog(HWND hwnd, HWND owner) {
    RECT anchor{};
    if (owner && GetWindowRect(owner, &anchor)) {
        const int x = anchor.left + ((anchor.right - anchor.left) - kDialogWidth) / 2;
        const int y = anchor.top + ((anchor.bottom - anchor.top) - kDialogHeight) / 2;
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

LRESULT CALLBACK dialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<ToolbarDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        state = static_cast<ToolbarDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    switch (message) {
    case WM_CREATE: {
        if (!state) {
            return -1;
        }

        HWND title = createControl(L"STATIC", L"Customize Toolbar", 0, 20, 16, 240, 22, hwnd, 0);
        HWND availableLabel = createControl(L"STATIC", L"Available:", 0, 20, 52, 160, 20, hwnd, 0);
        HWND toolbarLabel = createControl(L"STATIC", L"Toolbar:", 0, 320, 52, 160, 20, hwnd, 0);
        HWND available = createControl(L"LISTBOX", L"", WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY, 20, 76, 180, 190, hwnd, kAvailableListId, WS_EX_CLIENTEDGE);
        HWND toolbar = createControl(L"LISTBOX", L"", WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY, 320, 76, 180, 190, hwnd, kToolbarListId, WS_EX_CLIENTEDGE);
        HWND add = createControl(L"BUTTON", L"Add >", WS_TABSTOP, 220, 96, 80, 28, hwnd, kAddId);
        HWND remove = createControl(L"BUTTON", L"< Remove", WS_TABSTOP, 220, 132, 80, 28, hwnd, kRemoveId);
        HWND up = createControl(L"BUTTON", L"Up", WS_TABSTOP, 220, 186, 80, 28, hwnd, kUpId);
        HWND down = createControl(L"BUTTON", L"Down", WS_TABSTOP, 220, 222, 80, 28, hwnd, kDownId);
        HWND ok = createControl(L"BUTTON", L"Done", WS_TABSTOP | BS_DEFPUSHBUTTON, 392, 288, 76, 28, hwnd, kOkId);
        HWND cancel = createControl(L"BUTTON", L"Cancel", WS_TABSTOP, 476, 288, 76, 28, hwnd, kCancelId);
        if (!title || !availableLabel || !toolbarLabel || !available || !toolbar || !add || !remove || !up || !down || !ok || !cancel) {
            return -1;
        }
        refreshLists(hwnd, *state);
        return 0;
    }
    case WM_COMMAND:
        if (!state) {
            break;
        }
        if (LOWORD(wParam) == kAddId) {
            const int index = selectedListIndex(hwnd, kAvailableListId);
            const std::vector<ToolbarCommand> available = availableToolbarCommands();
            if (index >= 0 && index < static_cast<int>(available.size())) {
                addToolbarCommand(state->resultSettings.toolbarCommands, available[static_cast<std::size_t>(index)]);
                refreshLists(hwnd, *state, static_cast<int>(state->resultSettings.toolbarCommands.size()) - 1);
            }
            return 0;
        }
        if (LOWORD(wParam) == kRemoveId) {
            const int index = selectedListIndex(hwnd, kToolbarListId);
            if (index >= 0 && removeToolbarCommand(state->resultSettings.toolbarCommands, static_cast<std::size_t>(index))) {
                refreshLists(hwnd, *state, index);
            }
            return 0;
        }
        if (LOWORD(wParam) == kUpId || LOWORD(wParam) == kDownId) {
            const int index = selectedListIndex(hwnd, kToolbarListId);
            const int direction = LOWORD(wParam) == kUpId ? -1 : 1;
            if (index >= 0 && moveToolbarCommand(state->resultSettings.toolbarCommands, static_cast<std::size_t>(index), direction)) {
                refreshLists(hwnd, *state, index + direction);
            }
            return 0;
        }
        if (LOWORD(wParam) == kOkId) {
            state->resultSettings.toolbarCommands = normalizeToolbarCommands(std::move(state->resultSettings.toolbarCommands));
            state->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == kCancelId) {
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

bool registerDialogClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = dialogProc;
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

bool promptForToolbarCustomization(HWND owner, AppSettings& settings) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!registerDialogClass(instance)) {
        return false;
    }

    ToolbarDialogState state{};
    state.initialSettings = settings;
    state.resultSettings = settings;

    HWND dialog = CreateWindowExW(WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME,
                                  kDialogClassName,
                                  L"Customize Toolbar",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU,
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

    MSG message{};
    while (IsWindow(dialog)) {
        const int result = GetMessageW(&message, nullptr, 0, 0);
        if (result <= 0) {
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

    if (!state.accepted || state.resultSettings.toolbarCommands == state.initialSettings.toolbarCommands) {
        return false;
    }
    settings.toolbarCommands = std::move(state.resultSettings.toolbarCommands);
    return true;
}

} // namespace finderx::ui
