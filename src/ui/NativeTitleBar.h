#pragma once

#include "settings/AppSettings.h"

#include <windows.h>

namespace finderx::ui {

bool shouldUseDarkTitleBar(ThemeMode themeMode);
bool applyNativeTitleBarTheme(HWND hwnd, ThemeMode themeMode);

} // namespace finderx::ui
