#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace finderx {

inline constexpr float kDefaultFontSize = 13.0f;
inline constexpr float kDefaultIconSize = 14.0f;
inline constexpr float kMinFontSize = 11.0f;
inline constexpr float kMaxFontSize = 18.0f;
inline constexpr float kMinIconSize = 12.0f;
inline constexpr float kMaxIconSize = 24.0f;

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

struct FavoriteLocation {
    std::wstring label;
    std::wstring path;
};

struct AppSettings {
    float fontSize = kDefaultFontSize;
    float iconSize = kDefaultIconSize;
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
std::filesystem::path defaultSettingsPath();
SettingsLoadResult loadSettings(const std::wstring& homePath, const std::filesystem::path& path = defaultSettingsPath());
bool saveSettings(const AppSettings& settings, const std::filesystem::path& path = defaultSettingsPath());
std::wstring sortColumnName(SortColumn column);
std::wstring sortDirectionName(SortDirection direction);

} // namespace finderx
