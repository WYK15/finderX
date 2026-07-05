#pragma once

#include "settings/AppSettings.h"

#include <windows.h>

#include <string>

namespace finderx::ui {

struct SettingsDialogValues {
    std::wstring fontSizeText;
    std::wstring iconSizeText;
    std::wstring itemPaddingText;
    std::wstring themeModeText;
    std::wstring fontFamilyText;
    std::wstring contextMenuFontSizeText;
    bool showHiddenAndSystemItems = false;
    std::wstring windowWidthText;
    std::wstring windowHeightText;
    bool rememberWindowSize = false;
    std::wstring startupFolderText;
};

bool promptForSettings(HWND owner, AppSettings& settings, const std::wstring& currentFolder = L"");
void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings);
float settingsDialogBodyFontSize();
float settingsDialogTitleFontSize();
DWORD settingsDialogWindowStyle();
DWORD settingsDialogListBoxStyle();
DWORD settingsDialogComboBoxStyle();
int settingsDialogComboDroppedHeight(int itemHeight, int visibleItems);

} // namespace finderx::ui
