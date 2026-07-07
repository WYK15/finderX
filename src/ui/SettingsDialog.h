#pragma once

#include "settings/AppSettings.h"

#include <windows.h>

#include <string>
#include <vector>

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
    std::vector<ToolbarCommand> toolbarCommands;
    std::wstring languageModeText;
};

bool promptForSettings(HWND owner, AppSettings& settings, const std::wstring& currentFolder = L"");
void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings);
bool settingsDialogSettingsEqual(const AppSettings& left, const AppSettings& right);
float settingsDialogBodyFontSize();
float settingsDialogTitleFontSize();
DWORD settingsDialogWindowStyle();
DWORD settingsDialogListBoxStyle();
DWORD settingsDialogComboBoxStyle();
int settingsDialogComboDroppedHeight(int itemHeight, int visibleItems);

} // namespace finderx::ui
