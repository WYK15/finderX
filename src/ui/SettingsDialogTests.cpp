#include "ui/SettingsDialog.h"

#include "settings/AppSettings.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

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
    using finderx::addFavorite;
    using finderx::kMaxIconSize;
    using finderx::ui::SettingsDialogValues;
    using finderx::ui::applySettingsDialogValues;

    {
        AppSettings settings;
        applySettingsDialogValues(SettingsDialogValues{L"16", L"20", L"", L"Microsoft YaHei UI"}, settings);
        require(nearlyEqual(settings.fontSize, 16.0f), "font text 16 should apply font size 16");
        require(nearlyEqual(settings.iconSize, 20.0f), "icon text 20 should apply icon size 20");
        require(settings.fontFamily == L"Microsoft YaHei UI", "font family text should apply font family");
    }

    {
        AppSettings settings;
        settings.themeMode = ThemeMode::Dark;
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"light"}, settings);
        require(settings.themeMode == ThemeMode::Light, "theme text light should apply light theme");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"dark"}, settings);
        require(settings.themeMode == ThemeMode::Dark, "theme text dark should apply dark theme");
    }

    {
        AppSettings settings;
        settings.themeMode = ThemeMode::Dark;
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"unknown"}, settings);
        require(settings.themeMode == ThemeMode::Dark, "invalid theme text should keep previous theme");
    }

    {
        AppSettings settings;
        require(!settings.showHiddenAndSystemItems, "hidden/system setting should default off");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"", true}, settings);
        require(settings.showHiddenAndSystemItems, "checked hidden/system setting should apply");
        applySettingsDialogValues(SettingsDialogValues{L"", L"", L"", L"", false}, settings);
        require(!settings.showHiddenAndSystemItems, "unchecked hidden/system setting should apply");
    }

    {
        AppSettings settings;
        settings.fontSize = 15.0f;
        settings.iconSize = 18.0f;
        settings.fontFamily = L"Segoe UI";
        applySettingsDialogValues(SettingsDialogValues{L"large", L"19"}, settings);
        require(nearlyEqual(settings.fontSize, 15.0f), "invalid font text should keep previous font size");
        require(nearlyEqual(settings.iconSize, 19.0f), "valid icon text should still apply icon size");
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

    std::cout << "SettingsDialogTests passed\n";
    return 0;
}
