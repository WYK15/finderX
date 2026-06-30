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
    require(settings.iconSize == 14.0f, "default icon size should be 14");
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
    clampSettings(settings);
    require(settings.fontSize == kMinFontSize, "font size should clamp to min");
    require(settings.iconSize == kMaxIconSize, "icon size should clamp to max");

    settings.fontSize = 30.0f;
    settings.iconSize = 2.0f;
    clampSettings(settings);
    require(settings.fontSize == kMaxFontSize, "font size should clamp to max");
    require(settings.iconSize == kMinIconSize, "icon size should clamp to min");
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

void testSaveLoadRoundTrip() {
    const std::filesystem::path path = tempSettingsPath(L"finderx-settings-");
    AppSettings settings = makeDefaultSettings(L"C:\\Users\\Example");
    settings.fontSize = 16.0f;
    settings.iconSize = 20.0f;
    settings.sortColumn = SortColumn::Modified;
    settings.sortDirection = SortDirection::Descending;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "custom favorite should be added before save");

    require(saveSettings(settings, path), "settings should save");
    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "load result should report disk load");
    require(loaded.settings.fontSize == 16.0f, "font size should round trip");
    require(loaded.settings.iconSize == 20.0f, "icon size should round trip");
    require(loaded.settings.sortColumn == SortColumn::Modified, "sort column should round trip");
    require(loaded.settings.sortDirection == SortDirection::Descending, "sort direction should round trip");
    require(loaded.settings.favorites.size() == 5, "default plus custom favorites should round trip");
    require(loaded.settings.favorites.back().label == L"Projects", "custom favorite label should round trip");
    require(loaded.settings.favorites.back().path == L"D:\\Projects", "custom favorite path should round trip");

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
        stream << "  \"iconSize\": \"bad 20\",\n";
        stream << "  \"sortColumn\": \"unknown\",\n";
        stream << "  \"sortDirection\": \"unknown\",\n";
        stream << "  \"favorites\": []\n";
        stream << "}\n";
    }

    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "valid settings file with invalid fields should count as disk load");
    require(loaded.settings.fontSize == kDefaultFontSize, "invalid font field should keep default");
    require(loaded.settings.iconSize == kDefaultIconSize, "invalid icon field should keep default");
    require(loaded.settings.sortColumn == SortColumn::Name, "invalid sort column should keep default");
    require(loaded.settings.sortDirection == SortDirection::Ascending, "invalid sort direction should keep default");
    require(loaded.settings.favorites.size() == 4, "empty favorites array should keep defaults");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    testDefaults();
    testClamping();
    testFavoriteOperations();
    testSaveLoadRoundTrip();
    testMalformedSettingsFallback();
    testInvalidFieldsFallbackToDefaults();
    std::cout << "AppSettingsTests passed\n";
}
