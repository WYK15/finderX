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
    require(settings.previewFontFamily == L"Segoe UI", "default preview font family should be Segoe UI");
    require(settings.previewFontSize == kDefaultPreviewFontSize, "default preview font size should match text size");
    require(std::wstring(kFinderXVersion) == L"0.2.0", "FinderX version should be 0.2.0");
    require(settings.languageMode == LanguageMode::Chinese, "default language should be Chinese");
    require(settings.contextMenuFontSize == 12.0f, "default context menu font size should be 12");
    require(settings.iconSize == 14.0f, "default icon size should be 14");
    require(settings.itemPadding == kDefaultItemPadding, "default item padding should be compact");
    require(settings.wheelScrollPixels == kDefaultWheelScrollPixels, "default wheel scroll speed should be gentle");
    require(settings.modifiedColumnWidth == 150.0f, "default modified column width should be 150");
    require(settings.sizeColumnWidth == 80.0f, "default size column width should be 80");
    require(settings.kindColumnWidth == 120.0f, "default kind column width should be 120");
    require(settings.sidebarWidth == kDefaultSidebarWidth, "default sidebar width should match baseline");
    require(settings.windowWidth == 1188, "default window width should be 1188");
    require(settings.windowHeight == 768, "default window height should be 768");
    require(!settings.rememberWindowSize, "default should not remember window size");
    require(settings.startupFolder.empty(), "default startup folder should be empty");
    require(settings.themeMode == ThemeMode::Dark, "default theme should be dark");
    require(!settings.showHiddenAndSystemItems, "default should hide hidden and system items");
    require(settings.sortColumn == SortColumn::Name, "default sort column should be name");
    require(settings.sortDirection == SortDirection::Ascending, "default sort direction should be ascending");
    require(settings.toolbarCommands.size() == 6, "default toolbar should include six commands");
    require(settings.toolbarCommands[0] == ToolbarCommand::NewFolder, "default toolbar should start with new folder");
    require(settings.toolbarCommands[1] == ToolbarCommand::NewFile, "default toolbar should include new file");
    require(settings.toolbarCommands[4] == ToolbarCommand::PowerShell, "default toolbar should include PowerShell before search");
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
    settings.contextMenuFontSize = 3.0f;
    settings.previewFontSize = 3.0f;
    settings.iconSize = 99.0f;
    settings.itemPadding = 1.0f;
    settings.wheelScrollPixels = 1.0f;
    settings.modifiedColumnWidth = 20.0f;
    settings.sizeColumnWidth = 20.0f;
    settings.kindColumnWidth = 20.0f;
    settings.sidebarWidth = 20.0f;
    settings.windowWidth = 100;
    settings.windowHeight = 100;
    clampSettings(settings);
    require(settings.fontSize == kMinFontSize, "font size should clamp to min");
    require(settings.contextMenuFontSize == kMinContextMenuFontSize, "context menu font size should clamp to min");
    require(settings.previewFontSize == kMinPreviewFontSize, "preview font size should clamp to min");
    require(settings.iconSize == kMaxIconSize, "icon size should clamp to max");
    require(settings.itemPadding == kMinItemPadding, "item padding should clamp to min");
    require(settings.wheelScrollPixels == kMinWheelScrollPixels, "wheel scroll speed should clamp to min");
    require(settings.modifiedColumnWidth == kMinModifiedColumnWidth, "modified column width should clamp to min");
    require(settings.sizeColumnWidth == kMinSizeColumnWidth, "size column width should clamp to min");
    require(settings.kindColumnWidth == kMinKindColumnWidth, "kind column width should clamp to min");
    require(settings.sidebarWidth == kMinSidebarWidth, "sidebar width should clamp to min");
    require(settings.windowWidth == kMinWindowWidth, "window width should clamp to min");
    require(settings.windowHeight == kMinWindowHeight, "window height should clamp to min");

    settings.fontSize = 30.0f;
    settings.contextMenuFontSize = 30.0f;
    settings.previewFontSize = 40.0f;
    settings.iconSize = 2.0f;
    settings.itemPadding = 99.0f;
    settings.wheelScrollPixels = 999.0f;
    settings.modifiedColumnWidth = 999.0f;
    settings.sizeColumnWidth = 999.0f;
    settings.kindColumnWidth = 999.0f;
    settings.sidebarWidth = 999.0f;
    settings.windowWidth = 99999;
    settings.windowHeight = 99999;
    clampSettings(settings);
    require(settings.fontSize == kMaxFontSize, "font size should clamp to max");
    require(settings.contextMenuFontSize == kMaxContextMenuFontSize, "context menu font size should clamp to max");
    require(settings.previewFontSize == kMaxPreviewFontSize, "preview font size should clamp to max");
    require(settings.iconSize == kMinIconSize, "icon size should clamp to min");
    require(settings.itemPadding == kMaxItemPadding, "item padding should clamp to max");
    require(settings.wheelScrollPixels == kMaxWheelScrollPixels, "wheel scroll speed should clamp to max");
    require(settings.modifiedColumnWidth == kMaxModifiedColumnWidth, "modified column width should clamp to max");
    require(settings.sizeColumnWidth == kMaxSizeColumnWidth, "size column width should clamp to max");
    require(settings.kindColumnWidth == kMaxKindColumnWidth, "kind column width should clamp to max");
    require(settings.sidebarWidth == kMaxSidebarWidth, "sidebar width should clamp to max");
    require(settings.windowWidth == kMaxWindowWidth, "window width should clamp to max");
    require(settings.windowHeight == kMaxWindowHeight, "window height should clamp to max");
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
    settings.previewFontFamily = L"Cascadia Mono";
    settings.previewFontSize = 18.0f;
    settings.languageMode = LanguageMode::English;
    settings.contextMenuFontSize = 11.5f;
    settings.iconSize = 20.0f;
    settings.itemPadding = 14.0f;
    settings.wheelScrollPixels = 32.0f;
    settings.modifiedColumnWidth = 188.0f;
    settings.sizeColumnWidth = 96.0f;
    settings.kindColumnWidth = 164.0f;
    settings.sidebarWidth = 236.0f;
    settings.windowWidth = 1440;
    settings.windowHeight = 900;
    settings.rememberWindowSize = true;
    settings.startupFolder = L"D:\\Work";
    settings.themeMode = ThemeMode::Light;
    settings.showHiddenAndSystemItems = true;
    settings.sortColumn = SortColumn::Modified;
    settings.sortDirection = SortDirection::Descending;
    settings.toolbarCommands = {ToolbarCommand::Search, ToolbarCommand::PowerShell, ToolbarCommand::NewFile, ToolbarCommand::NewFolder};
    settings.contextMenuTools.push_back({L"Open in VS Code", L"C:\\Tools\\Code.exe", L"\"{path}\"", true, true});
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "custom favorite should be added before save");

    require(saveSettings(settings, path), "settings should save");
    const SettingsLoadResult loaded = loadSettings(L"C:\\Users\\Example", path);
    require(loaded.loadedFromDisk, "load result should report disk load");
    require(loaded.settings.fontSize == 16.0f, "font size should round trip");
    require(loaded.settings.fontFamily == L"Microsoft YaHei UI", "font family should round trip");
    require(loaded.settings.previewFontFamily == L"Cascadia Mono", "preview font family should round trip");
    require(loaded.settings.previewFontSize == 18.0f, "preview font size should round trip");
    require(loaded.settings.languageMode == LanguageMode::English, "language should round trip");
    require(loaded.settings.contextMenuFontSize == 11.5f, "context menu font size should round trip");
    require(loaded.settings.iconSize == 20.0f, "icon size should round trip");
    require(loaded.settings.itemPadding == 14.0f, "item padding should round trip");
    require(loaded.settings.wheelScrollPixels == 32.0f, "wheel scroll speed should round trip");
    require(loaded.settings.modifiedColumnWidth == 188.0f, "modified column width should round trip");
    require(loaded.settings.sizeColumnWidth == 96.0f, "size column width should round trip");
    require(loaded.settings.kindColumnWidth == 164.0f, "kind column width should round trip");
    require(loaded.settings.sidebarWidth == 236.0f, "sidebar width should round trip");
    require(loaded.settings.windowWidth == 1440, "window width should round trip");
    require(loaded.settings.windowHeight == 900, "window height should round trip");
    require(loaded.settings.rememberWindowSize, "remember window size should round trip");
    require(loaded.settings.startupFolder == L"D:\\Work", "startup folder should round trip");
    require(loaded.settings.themeMode == ThemeMode::Light, "theme mode should round trip");
    require(loaded.settings.showHiddenAndSystemItems, "hidden/system visibility should round trip");
    require(loaded.settings.sortColumn == SortColumn::Modified, "sort column should round trip");
    require(loaded.settings.sortDirection == SortDirection::Descending, "sort direction should round trip");
    require(loaded.settings.toolbarCommands.size() == 4, "toolbar commands should round trip");
    require(loaded.settings.toolbarCommands[0] == ToolbarCommand::Search, "toolbar command order should round trip");
    require(loaded.settings.toolbarCommands[1] == ToolbarCommand::PowerShell, "toolbar PowerShell command should round trip");
    require(loaded.settings.toolbarCommands[2] == ToolbarCommand::NewFile, "toolbar new file command should round trip");
    require(loaded.settings.toolbarCommands[3] == ToolbarCommand::NewFolder, "toolbar new folder command should round trip");
    require(loaded.settings.contextMenuTools.size() == 1, "context menu tools should round trip");
    require(loaded.settings.contextMenuTools[0].label == L"Open in VS Code", "context menu tool label should round trip");
    require(loaded.settings.contextMenuTools[0].executablePath == L"C:\\Tools\\Code.exe", "context menu tool executable should round trip");
    require(loaded.settings.contextMenuTools[0].arguments == L"\"{path}\"", "context menu tool arguments should round trip");
    require(loaded.settings.contextMenuTools[0].appliesToFiles, "context menu tool file flag should round trip");
    require(loaded.settings.contextMenuTools[0].appliesToFolders, "context menu tool folder flag should round trip");
    require(loaded.settings.favorites.size() == 5, "default plus custom favorites should round trip");
    require(loaded.settings.favorites.back().label == L"Projects", "custom favorite label should round trip");
    require(loaded.settings.favorites.back().path == L"D:\\Projects", "custom favorite path should round trip");

    std::filesystem::remove(path);
}

void testNormalizeToolbarCommands() {
    std::vector<ToolbarCommand> normalized = normalizeToolbarCommands({
        ToolbarCommand::NewFolder,
        ToolbarCommand::NewFolder,
        ToolbarCommand::PowerShell,
        ToolbarCommand::Search,
        ToolbarCommand::Settings,
    });

    require(normalized.size() == 4, "duplicate toolbar commands should be removed");
    require(normalized[0] == ToolbarCommand::NewFolder, "normalization should keep first command");
    require(normalized[1] == ToolbarCommand::PowerShell, "normalization should preserve PowerShell command order");
    require(normalized[2] == ToolbarCommand::Search, "normalization should preserve remaining order");
    require(normalized[3] == ToolbarCommand::Settings, "normalization should keep later unique commands");

    normalized = normalizeToolbarCommands({});
    require(normalized == defaultToolbarCommands(), "empty toolbar should fall back to defaults");
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
        stream << "  \"previewFontFamily\": \"\",\n";
        stream << "  \"previewFontSize\": \"bad 18\",\n";
        stream << "  \"iconSize\": \"bad 20\",\n";
        stream << "  \"itemPadding\": \"bad 14\",\n";
        stream << "  \"wheelScrollPixels\": \"bad 24\",\n";
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
    require(loaded.settings.previewFontFamily == L"Segoe UI", "empty preview font family should keep default");
    require(loaded.settings.previewFontSize == kDefaultPreviewFontSize, "invalid preview font size should keep default");
    require(loaded.settings.iconSize == kDefaultIconSize, "invalid icon field should keep default");
    require(loaded.settings.itemPadding == kDefaultItemPadding, "invalid item padding field should keep default");
    require(loaded.settings.wheelScrollPixels == kDefaultWheelScrollPixels, "invalid wheel scroll speed should keep default");
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
    testNormalizeToolbarCommands();
    testEmptyFavoritesRoundTrip();
    testFavoritesWithStructuralCharactersRoundTrip();
    testMalformedSettingsFallback();
    testInvalidFieldsFallbackToDefaults();
    testAbsentFavoritesFallbackToDefaults();
    std::cout << "AppSettingsTests passed\n";
}
