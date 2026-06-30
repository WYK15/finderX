#pragma once

#include "settings/AppSettings.h"

#include <windows.h>

#include <string>

namespace finderx::ui {

struct SettingsDialogValues {
    std::wstring fontSizeText;
    std::wstring iconSizeText;
};

bool promptForSettings(HWND owner, AppSettings& settings);
void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings);

} // namespace finderx::ui
