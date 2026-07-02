#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace finderx {

inline constexpr float kDefaultFontSize = 13.0f;
inline constexpr float kDefaultIconSize = 14.0f;
inline constexpr wchar_t kDefaultFontFamily[] = L"Segoe UI";
inline constexpr float kMinFontSize = 11.0f;
inline constexpr float kMaxFontSize = 18.0f;
inline constexpr float kMinIconSize = 12.0f;
inline constexpr float kMaxIconSize = 24.0f;
inline constexpr float kDefaultModifiedColumnWidth = 150.0f;
inline constexpr float kDefaultSizeColumnWidth = 80.0f;
inline constexpr float kDefaultKindColumnWidth = 120.0f;
inline constexpr float kMinModifiedColumnWidth = 96.0f;
inline constexpr float kMaxModifiedColumnWidth = 280.0f;
inline constexpr float kMinSizeColumnWidth = 56.0f;
inline constexpr float kMaxSizeColumnWidth = 160.0f;
inline constexpr float kMinKindColumnWidth = 80.0f;
inline constexpr float kMaxKindColumnWidth = 260.0f;
inline constexpr int kDefaultWindowWidth = 1188;
inline constexpr int kDefaultWindowHeight = 768;
inline constexpr int kMinWindowWidth = 900;
inline constexpr int kMinWindowHeight = 560;
inline constexpr int kMaxWindowWidth = 3840;
inline constexpr int kMaxWindowHeight = 2160;

enum class SortColumn {
    Name,
    Modified,
    Size,
    Kind
};

enum class SortDirection {
    Ascending,
    Descending
};

enum class ThemeMode {
    Light,
    Dark
};

struct FavoriteLocation {
    std::wstring label;
    std::wstring path;
};

struct AppSettings {
    float fontSize = kDefaultFontSize;
    std::wstring fontFamily = kDefaultFontFamily;
    float iconSize = kDefaultIconSize;
    float modifiedColumnWidth = kDefaultModifiedColumnWidth;
    float sizeColumnWidth = kDefaultSizeColumnWidth;
    float kindColumnWidth = kDefaultKindColumnWidth;
    int windowWidth = kDefaultWindowWidth;
    int windowHeight = kDefaultWindowHeight;
    bool rememberWindowSize = false;
    ThemeMode themeMode = ThemeMode::Dark;
    bool showHiddenAndSystemItems = false;
    SortColumn sortColumn = SortColumn::Name;
    SortDirection sortDirection = SortDirection::Ascending;
    std::vector<FavoriteLocation> favorites;
};

struct SettingsLoadResult {
    AppSettings settings;
    bool loadedFromDisk = false;
};

AppSettings makeDefaultSettings(const std::wstring& homePath);
void clampSettings(AppSettings& settings);
bool addFavorite(AppSettings& settings, std::wstring label, std::wstring path);
bool removeFavorite(AppSettings& settings, const std::wstring& path);
bool containsFavorite(const AppSettings& settings, const std::wstring& path);
bool renameFavorite(AppSettings& settings, std::size_t index, std::wstring label);
bool moveFavorite(AppSettings& settings, std::size_t index, int direction);
std::filesystem::path defaultSettingsPath();
SettingsLoadResult loadSettings(const std::wstring& homePath, const std::filesystem::path& path = defaultSettingsPath());
bool saveSettings(const AppSettings& settings, const std::filesystem::path& path = defaultSettingsPath());
std::wstring sortColumnName(SortColumn column);
std::wstring sortDirectionName(SortDirection direction);
std::wstring themeModeName(ThemeMode mode);

} // namespace finderx
