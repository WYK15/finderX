#include "ui/NativeTitleBar.h"

#include <dwmapi.h>

namespace finderx::ui {

namespace {

constexpr DWORD kDwmUseImmersiveDarkMode = 20;
constexpr DWORD kDwmUseImmersiveDarkModeBefore20H1 = 19;

bool setDarkModeAttribute(HWND hwnd, DWORD attribute, BOOL enabled) {
    return SUCCEEDED(DwmSetWindowAttribute(hwnd, attribute, &enabled, sizeof(enabled)));
}

} // namespace

bool shouldUseDarkTitleBar(ThemeMode themeMode) {
    return themeMode == ThemeMode::Dark;
}

bool applyNativeTitleBarTheme(HWND hwnd, ThemeMode themeMode) {
    if (!hwnd) {
        return false;
    }

    const BOOL enabled = shouldUseDarkTitleBar(themeMode) ? TRUE : FALSE;
    if (setDarkModeAttribute(hwnd, kDwmUseImmersiveDarkMode, enabled)) {
        return true;
    }
    return setDarkModeAttribute(hwnd, kDwmUseImmersiveDarkModeBefore20H1, enabled);
}

} // namespace finderx::ui
