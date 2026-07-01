#include "settings/AppSettings.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

std::filesystem::path tempSettingsPath(const wchar_t* name) {
    wchar_t tempPath[MAX_PATH]{};
    const DWORD length = GetTempPathW(static_cast<DWORD>(std::size(tempPath)), tempPath);
    require(length > 0 && length < std::size(tempPath), "temp path should be available");
    return std::filesystem::path(tempPath) / (std::wstring(name) + std::to_wstring(GetTickCount64()) + L".json");
}

void testDefaults() {
    const AppSettings settings = makeDefaultSettings(L"C:\\Users\\Example");
    require(settings.fontSize == 13.0f, "default font size should be 13");
    require(settings.fontFamily == L"Segoe UI", "default font family should be Segoe UI");
    require(settings.iconSize == 14.0f, "default icon size should be 14");
    require(settings.modifiedColumnWidth == 150.0f, "default modified column width should be 150");
    require(settings.sizeColumnWidth == 80.0f, "default size column width should be 80");
    require(settings.kindColumnWidth == 120.0f, "default kind column width should be 120");
    require(settings.themeMode == ThemeMode::Dark, "default theme should be dark");
    require(!settings.showHiddenAndSystemItems, "default should hide hidden and system items");
    require(settings.sortColumn == SortColumn::Name, "default sort column should be name");
    require(settings.sortDirection == SortDirection::Ascending, "default sort direction should be ascending");
    require(settings.favorites.size() == 4, "default settings should include four favorites");
    require(settings.favorites[0].label == L"Home", "first default favorite should be Home");
    require(settings.favorites[0].path == L"C:\\Users\\Example", "home favorite should use home path");
    require(settings.favorites[1].label == L"Desktop", "second default favorite should be Desktop");
    require(settings.favorites[1].path == L"C:\\Users\\Example\\Desktop", "desktop favorite should derive from home");
    require(settings.favorites[2].label == L"Documents", "third default favorite should be Documents");
    require(settings.favorites[2].path == L"C:\\Users\\Example\\Documents", "documents favorite should derive from home");
    require(settings.favorites[3].label == L"Downloads", "fourth default favorite should be Downloads");
    require(settings.favorites[3].path == L"C:\\Users\\Example\\Downloads", "downloads favorite should derive from home");
}

void testClamping() {
    AppSettings settings;
    settings.fontSize = 3.0f;
    settings.iconSize = 99.0f;
    settings.modifiedColumnWidth = 20.0f;
    settings.sizeColumnWidth = 20.0f;
    settings.kindColumnWidth = 20.0f;
    clampSettings(settings);
    require(settings.fontSize == kMinFontSize, "font size should clamp to min");
    require(settings.iconSize == kMaxIconSize, "icon size should clamp to max");
    require(settings.modifiedColumnWidth == kMinModifiedColumnWidth, "modified column width should clamp to min");
    require(settings.sizeColumnWidth == kMinSizeColumnWidth, "size column width should clamp to min");
    require(settings.kindColumnWidth == kMinKindColumnWidth, "kind column width should clamp to min");

    settings.fontSize = 30.0f;
    settings.iconSize = 2.0f;
    settings.modifiedColumnWidth = 999.0f;
    settings.sizeColumnWidth = 999.0f;
    settings.kindColumnWidth = 999.0f;
    clampSettings(settings);
    require(settings.fontSize == kMaxFontSize, "font size should clamp to max");
    require(settings.iconSize == kMinIconSize, "icon size should clamp to min");
    require(settings.modifiedColumnWidth == kMaxModifiedColumnWidth, "modified column width should clamp to max");
    require(settings.sizeColumnWidth == kMaxSizeColumnWidth, "size column width should clamp to max");
    require(settings.kindColumnWidth == kMaxKindColumnWidth, "kind column width should clamp to max");
}

void testFavoriteOperations() {
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "new favorite should be added");
    require(settings.favorites.size() == 1, "one favorite should be present");
    require(containsFavorite(settings, L"d:\\projects"), "favorite should be found case-insensitively");
    require(!addFavorite(settings, L"Projects Again", L"d:\\projects"), "duplicate favorite should not be added case-insensitively");
    require(settings.favorites.size() == 1, "duplicate favorite should not change count");
    require(removeFavorite(settings, L"D:\\PROJECTS"), "favorite should be removed case-insensitively");
    require(settings.favorites.empty(), "favorite list should be empty after removal");
    require(!containsFavorite(settings, L"D:\\Projects"), "removed favorite should not be found");
    require(!removeFavorite(settings, L"D:\\Missing"), "removing missing favorite should report false");
}

void testRenameFavoriteByIndex() {
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "favorite should exist before rename");

    require(renameFavorite(settings, 0, L"Work"), "renameFavorite should rename valid index");
    require(settings.favorites[0].label == L"Work", "renameFavorite should update label");
    require(settings.favorites[0].path == L"D:\\Projects", "renameFavorite should keep path");
    require(!renameFavorite(settings, 9, L"Missing"), "renameFavorite should reject invalid index");
}

void testRenameFavoriteEmptyLabelFallsBackToPathName() {
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "favorite should exist before fallback rename");

    require(renameFavorite(settings, 0, L""), "empty rename should still update valid favorite");
    require(settings.favorites[0].label == L"Projects", "empty label should fall back to final path component");
}

void testMoveFavoriteByIndex() {
    AppSettings settings;
    require(addFavorite(settings, L"One", L"D:\\One"), "first favorite should be added");
    require(addFavorite(settings, L"Two", L"D:\\Two"), "second favorite should be added");
    require(addFavorite(settings, L"Three", L"D:\\Three"), "third favorite should be added");

    require(moveFavorite(settings, 2, -1), "moveFavorite should move item up");
    require(settings.favorites[1].label == L"Three", "moved favorite should occupy previous slot");
    require(moveFavorite(settings, 1, 1), "moveFavorite should move item down");
    require(settings.favorites[2].label == L"Three", "moved favorite should occupy next slot");
    require(!moveFavorite(settings, 0, -1), "moveFavorite should reject first item moving up");
    require(!moveFavorite(settings, 2, 1), "moveFavorite should reject last item moving down");
}

void testSaveLoadRoundTrip() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-");
    AppSettings settings = makeDefaultSettings(L"C:\\Users\\Example");
    settings.fontSize = 16.0f;
    settings.fontFamily = L"Microsoft YaHei UI";
    settings.iconSize = 20.0f;
    settings.modifiedColumnWidth = 188.0f;
    settings.sizeColumnWidth = 96.0f;
    settings.kindColumnWidth = 164.0f;
    settings.themeMode = ThemeMode::Light;
    settings.showHiddenAndSystemItems = true;
    settings.sortColumn = SortColumn::Modified;
    settings.sortDirection = SortDirection::Descending;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "custom favorite should be added before save");

    require(saveSettings(settings, path), "settings should save");
    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "load result should report disk load");
    require(loaded.settings.fontSize == 16.0f, "font size should round trip");
    require(loaded.settings.fontFamily == L"Microsoft YaHei UI", "font family should round trip");
    require(loaded.settings.iconSize == 20.0f, "icon size should round trip");
    require(loaded.settings.modifiedColumnWidth == 188.0f, "modified column width should round trip");
    require(loaded.settings.sizeColumnWidth == 96.0f, "size column width should round trip");
    require(loaded.settings.kindColumnWidth == 164.0f, "kind column width should round trip");
    require(loaded.settings.themeMode == ThemeMode::Light, "theme mode should round trip");
    require(loaded.settings.showHiddenAndSystemItems, "hidden/system visibility should round trip");
    require(loaded.settings.sortColumn == SortColumn::Modified, "sort column should round trip");
    require(loaded.settings.sortDirection == SortDirection::Descending, "sort direction should round trip");
    require(loaded.settings.favorites.size() == 5, "default plus custom favorites should round trip");
    require(loaded.settings.favorites.back().label == L"Projects", "custom favorite label should round trip");
    require(loaded.settings.favorites.back().path == L"D:\\Projects", "custom favorite path should round trip");

    std::filesystem::remove(path);
}

void testEmptyFavoritesRoundTrip() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-empty-favorites-");
    AppSettings settings = makeDefaultSettings(L"C:\\Users\\Example");
    settings.favorites.clear();

    require(saveSettings(settings, path), "settings with empty favorites should save");
    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "empty favorites load should report disk load");
    require(loaded.settings.favorites.empty(), "persisted empty favorites should load as empty");

    std::filesystem::remove(path);
}

void testFavoritesWithStructuralCharactersRoundTrip() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-structural-favorites-");
    AppSettings settings;
    require(addFavorite(settings, L"Proj]ects", L"D:\\A}B"), "favorite with structural characters should be added");

    require(saveSettings(settings, path), "settings with structural characters should save");
    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "structural character settings should report disk load");
    require(loaded.settings.favorites.size() == 1, "structural character favorite should round trip");
    require(loaded.settings.favorites[0].label == L"Proj]ects", "favorite label with bracket should round trip");
    require(loaded.settings.favorites[0].path == L"D:\\A}B", "favorite path with brace should round trip");

    std::filesystem::remove(path);
}

void testMalformedSettingsFallback() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-bad-");
    {
        std::ofstream stream(path, std::ios::binary);
        stream << "{ not-json";
    }

    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(!loaded.loadedFromDisk, "malformed settings should not count as disk load");
    require(loaded.settings.fontSize == 13.0f, "malformed settings should use default font size");
    require(loaded.settings.favorites.size() == 4, "malformed settings should use default favorites");

    std::filesystem::remove(path);
}

void testInvalidFieldsFallbackToDefaults() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-invalid-");
    {
        std::ofstream stream(path, std::ios::binary);
        stream << "{\n";
        stream << "  \"fontSize\": \"bad 16\",\n";
        stream << "  \"fontFamily\": \"\",\n";
        stream << "  \"iconSize\": \"bad 20\",\n";
        stream << "  \"themeMode\": \"unknown\",\n";
        stream << "  \"sortColumn\": \"unknown\",\n";
        stream << "  \"sortDirection\": \"unknown\",\n";
        stream << "  \"favorites\": []\n";
        stream << "}\n";
    }

    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "valid settings file with invalid fields should count as disk load");
    require(loaded.settings.fontSize == kDefaultFontSize, "invalid font field should keep default");
    require(loaded.settings.fontFamily == L"Segoe UI", "empty font family should keep default");
    require(loaded.settings.iconSize == kDefaultIconSize, "invalid icon field should keep default");
    require(loaded.settings.themeMode == ThemeMode::Dark, "invalid theme mode should keep default");
    require(loaded.settings.sortColumn == SortColumn::Name, "invalid sort column should keep default");
    require(loaded.settings.sortDirection == SortDirection::Ascending, "invalid sort direction should keep default");
    require(loaded.settings.favorites.empty(), "present empty favorites array should load as empty");

    std::filesystem::remove(path);
}

void testAbsentFavoritesFallbackToDefaults() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-absent-favorites-");
    {
        std::ofstream stream(path, std::ios::binary);
        stream << "{\n";
        stream << "  \"fontSize\": 16,\n";
        stream << "  \"iconSize\": 20\n";
        stream << "}\n";
    }

    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "settings without favorites should report disk load");
    require(loaded.settings.favorites.size() == 4, "absent favorites should keep defaults");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    testDefaults();
    testClamping();
    testFavoriteOperations();
    testRenameFavoriteByIndex();
    testRenameFavoriteEmptyLabelFallsBackToPathName();
    testMoveFavoriteByIndex();
    testSaveLoadRoundTrip();
    testEmptyFavoritesRoundTrip();
    testFavoritesWithStructuralCharactersRoundTrip();
    testMalformedSettingsFallback();
    testInvalidFieldsFallbackToDefaults();
    testAbsentFavoritesFallbackToDefaults();
    std::cout << "AppSettingsTests passed\n";
}
