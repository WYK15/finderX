#include "ui/SettingsDialog.h"

#include "settings/AppSettings.h"
#include "ui/Localization.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool nearlyEqual(float left, float right) {
    return std::fabs(left - right) < 0.001f;
}

} // namespace

int main() {
    using finderx::AppSettings;
    using finderx::ThemeMode;
    using finderx::ToolbarCommand;
    using finderx::addFavorite;
    using finderx::kMaxIconSize;
    using finderx::ui::SettingsDialogValues;
    using finderx::ui::StringId;
    using finderx::ui::applySettingsDialogValues;
    using finderx::ui::settingsDialogBodyFontSize;
    using finderx::ui::settingsDialogComboBoxStyle;
    using finderx::ui::settingsDialogComboDroppedHeight;
    using finderx::ui::settingsDialogListBoxStyle;
    using finderx::ui::settingsDialogSettingsEqual;
    using finderx::ui::settingsDialogTitleFontSize;
    using finderx::ui::settingsDialogWindowStyle;
    using finderx::ui::tr;

    {
        require(nearlyEqual(settingsDialogBodyFontSize(), 10.5f), "settings body font should stay compact and independent");
        require(nearlyEqual(settingsDialogTitleFontSize(), 13.0f), "settings title font should stay compact");
        require((settingsDialogWindowStyle() & WS_THICKFRAME) != 0, "settings dialog should be resizable");
        require((settingsDialogWindowStyle() & WS_MAXIMIZEBOX) != 0, "settings dialog should support maximize");
        require((settingsDialogListBoxStyle() & LBS_NOINTEGRALHEIGHT) != 0, "settings list boxes should keep the exact layout height");
        require((settingsDialogComboBoxStyle() & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) != 0, "settings combo boxes should use owner draw");
        require((settingsDialogComboBoxStyle() & WS_VSCROLL) != 0, "settings combo boxes should expose a scrollbar for long lists");
        require(settingsDialogComboDroppedHeight(28, 8) >= 28 * 9, "settings combo dropdown should show multiple items");
        require(settingsDialogComboDroppedHeight(28, 16) >= 28 * 17, "font family dropdown should show a tall scrollable list");
    }

    {
        AppSettings settings;
        applySettingsDialogValues(SettingsDialogValues{L"16", L"20", L"14", L"", L"Microsoft YaHei UI", L"11.5", false, L"1440", L"900", true, L"D:\\Work"}, settings);
        require(nearlyEqual(settings.fontSize, 16.0f), "font text 16 should apply font size 16");
        require(nearlyEqual(settings.contextMenuFontSize, 11.5f), "context menu font text should apply context menu font size");
        require(nearlyEqual(settings.iconSize, 20.0f), "icon text 20 should apply icon size 20");
        require(nearlyEqual(settings.itemPadding, 14.0f), "item padding text should apply item padding");
        require(settings.fontFamily == L"Microsoft YaHei UI", "font family text should apply font family");
        require(settings.windowWidth == 1440, "window width text should apply");
        require(settings.windowHeight == 900, "window height text should apply");
        require(settings.startupFolder == L"D:\\Work", "startup folder text should apply");
    }

    {
        AppSettings settings;
        settings.themeMode = ThemeMode::Dark;
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"light"}, settings);
        require(settings.themeMode == ThemeMode::Light, "theme text light should apply light theme");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"dark"}, settings);
        require(settings.themeMode == ThemeMode::Dark, "theme text dark should apply dark theme");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", std::wstring(tr(StringId::Light))}, settings);
        require(settings.themeMode == ThemeMode::Light, "localized light theme text should apply light theme");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", std::wstring(tr(StringId::Dark))}, settings);
        require(settings.themeMode == ThemeMode::Dark, "localized dark theme text should apply dark theme");
    }

    {
        AppSettings settings;
        SettingsDialogValues values;
        values.languageModeText = L"English";
        applySettingsDialogValues(values, settings);
        require(settings.languageMode == finderx::LanguageMode::English, "English language text should apply English language");
        values.languageModeText = L"中文";
        applySettingsDialogValues(values, settings);
        require(settings.languageMode == finderx::LanguageMode::Chinese, "Chinese language text should apply Chinese language");
    }

    {
        AppSettings settings;
        settings.themeMode = ThemeMode::Dark;
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"unknown"}, settings);
        require(settings.themeMode == ThemeMode::Dark, "invalid theme text should keep previous theme");
    }

    {
        AppSettings settings;
        require(!settings.showHiddenAndSystemItems, "hidden/system setting should default off");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"", L"", L"", true}, settings);
        require(settings.showHiddenAndSystemItems, "checked hidden/system setting should apply");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"", L"", L"", false}, settings);
        require(!settings.showHiddenAndSystemItems, "unchecked hidden/system setting should apply");
    }

    {
        AppSettings settings;
        require(!settings.rememberWindowSize, "remember window size should default off");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"", L"", L"", false, L"", L"", true}, settings);
        require(settings.rememberWindowSize, "checked remember window size should apply");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"", L"", L"", false, L"", L"", false}, settings);
        require(!settings.rememberWindowSize, "unchecked remember window size should apply");
    }

    {
        AppSettings settings;
        settings.fontSize = 15.0f;
        settings.contextMenuFontSize = 12.0f;
        settings.iconSize = 18.0f;
        settings.itemPadding = 8.0f;
        settings.windowWidth = 1280;
        settings.windowHeight = 720;
        settings.startupFolder = L"D:\\Keep";
        settings.fontFamily = L"Segoe UI";
        applySettingsDialogValues(SettingsDialogValues{L"large", L"19", L"wide", L"", L"", L"tiny", false, L"wide", L"tall", false, L""}, settings);
        require(nearlyEqual(settings.fontSize, 15.0f), "invalid font text should keep previous font size");
        require(nearlyEqual(settings.contextMenuFontSize, 12.0f), "invalid context menu font text should keep previous font size");
        require(nearlyEqual(settings.iconSize, 19.0f), "valid icon text should still apply icon size");
        require(nearlyEqual(settings.itemPadding, 8.0f), "invalid item padding text should keep previous item padding");
        require(settings.windowWidth == 1280, "invalid window width should keep previous width");
        require(settings.windowHeight == 720, "invalid window height should keep previous height");
        require(settings.startupFolder.empty(), "empty startup folder should clear startup override");
        require(settings.fontFamily == L"Segoe UI", "empty font family text should keep previous font family");
    }

    {
        AppSettings settings;
        applySettingsDialogValues(SettingsDialogValues{L"14", L"99"}, settings);
        require(nearlyEqual(settings.iconSize, kMaxIconSize), "icon text 99 should clamp to max icon size");
    }

    {
        AppSettings settings;
        require(addFavorite(settings, L"Projects", L"D:\\Projects"), "favorite should exist before applying dialog values");
        applySettingsDialogValues(SettingsDialogValues{L"16", L"20"}, settings);
        require(settings.favorites.size() == 1, "applying size values should preserve favorites");
        require(settings.favorites[0].label == L"Projects", "applying size values should preserve favorite label");
    }

    {
        AppSettings settings;
        settings.toolbarCommands = {ToolbarCommand::Search};
        SettingsDialogValues values;
        values.toolbarCommands = {ToolbarCommand::Settings, ToolbarCommand::PowerShell, ToolbarCommand::Search};
        applySettingsDialogValues(values, settings);
        require(settings.toolbarCommands.size() == 3, "settings values should apply toolbar command count");
        require(settings.toolbarCommands[0] == ToolbarCommand::Settings, "settings values should apply toolbar order");
        require(settings.toolbarCommands[1] == ToolbarCommand::PowerShell, "settings values should apply PowerShell toolbar command");
        require(settings.toolbarCommands[2] == ToolbarCommand::Search, "settings values should preserve search command");
    }

    {
        AppSettings left;
        AppSettings right = left;
        right.toolbarCommands = {ToolbarCommand::NewFolder, ToolbarCommand::NewFile, ToolbarCommand::Settings, ToolbarCommand::PowerShell, ToolbarCommand::Search};
        require(!settingsDialogSettingsEqual(left, right), "settings equality should detect toolbar command changes");
    }

    std::cout << "SettingsDialogTests passed\n";
    return 0;
}
