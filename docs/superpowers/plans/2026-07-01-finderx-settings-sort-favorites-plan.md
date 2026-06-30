# FinderX Settings, Sorting, And Favorites Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add global settings, persisted favorites, sortable columns, and configurable font/icon sizes to FinderX.

**Architecture:** Add a small settings module for persistence and favorite management, a focused sorter module for file ordering, and then wire those models into `MainWindow`, `SidebarModel`, `FinderChrome`, and `FinderListView`. Keep filesystem enumeration separate from user-selected sort order; UI code should consume already-loaded nodes plus settings state.

**Tech Stack:** C++20, Win32 API, Direct2D/DirectWrite, CMake/CTest, existing FinderX model/navigation/ui libraries.

---

## File Structure

- Create `src/settings/AppSettings.h` and `src/settings/AppSettings.cpp`
  - Own global settings defaults, clamping, favorites, fixed-shape JSON save/load, and `%APPDATA%\FinderX\settings.json` path resolution.
- Create `src/settings/AppSettingsTests.cpp`
  - Covers defaults, clamping, favorites, malformed file fallback, and JSON round trip using a temp settings file.
- Create `src/fs/FileSorter.h` and `src/fs/FileSorter.cpp`
  - Owns folder-first sorting by name, modified ticks, size bytes, and kind.
- Create `src/fs/FileSorterTests.cpp`
  - Covers all sort columns and directions plus folder-first behavior.
- Modify `src/model/FileNode.h`
  - Add raw sortable metadata fields: `modifiedTicks` and `sizeBytes`.
- Modify `src/fs/DirectoryLoader.cpp`, `src/fs/DriveEnumerator.cpp`, and current tests
  - Fill raw metadata when loading real files and drive nodes.
- Modify `src/navigation/SidebarModel.h`, `src/navigation/SidebarModel.cpp`, and `src/navigation/NavigationHistoryTests.cpp`
  - Add sidebar item roles and section headers; build Favorites and Locations from settings favorites.
- Modify `src/ui/FinderChrome.h` and `src/ui/FinderChrome.cpp`
  - Draw grouped sidebar section headers, toolbar Sort/Settings controls, active sort header arrows, and hit-test sort/settings/header/sidebar rows.
- Modify `src/ui/FinderListView.h`, `src/ui/FinderListView.cpp`, and `src/ui/FinderListViewTests.cpp`
  - Add configurable style for font/icon size and derived row height/hit testing.
- Create `src/ui/SettingsDialog.h`, `src/ui/SettingsDialog.cpp`, and `src/ui/SettingsDialogTests.cpp`
  - Small modal settings dialog plus pure parsing/clamping helpers for tests.
- Modify `src/ui/RenderContext.h` and `src/ui/RenderContext.cpp`
  - Recreate DirectWrite formats when font size changes.
- Modify `src/ui/MainWindow.h` and `src/ui/MainWindow.cpp`
  - Load/save settings, apply sorting, manage favorites, route new toolbar/header/sidebar context commands, and update visual style.
- Modify `CMakeLists.txt`
  - Add `finderx_settings`, `finderx_settings_tests`, `finderx_file_sorter_tests`, and `finderx_settings_dialog_tests`; link new modules into the app and existing tests as needed.

---

### Task 1: App Settings Model And Persistence

**Files:**
- Create: `src/settings/AppSettings.h`
- Create: `src/settings/AppSettings.cpp`
- Create: `src/settings/AppSettingsTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing settings tests**

Create `src/settings/AppSettingsTests.cpp`:

```cpp
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

void testDefaultsAndClamping() {
    AppSettings settings = makeDefaultSettings(L"C:\\Users\\Example");
    require(settings.fontSize == 13.0f, "default font size should be 13");
    require(settings.iconSize == 14.0f, "default icon size should be 14");
    require(settings.sortColumn == SortColumn::Name, "default sort column should be name");
    require(settings.sortDirection == SortDirection::Ascending, "default sort direction should be ascending");
    require(settings.favorites.size() == 4, "default settings should include four favorites");
    require(settings.favorites[0].label == L"Home", "first default favorite should be Home");
    require(settings.favorites[1].path == L"C:\\Users\\Example\\Desktop", "desktop favorite should derive from home");

    settings.fontSize = 3.0f;
    settings.iconSize = 99.0f;
    clampSettings(settings);
    require(settings.fontSize == kMinFontSize, "font size should clamp to min");
    require(settings.iconSize == kMaxIconSize, "icon size should clamp to max");
}

void testFavoriteOperations() {
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "new favorite should be added");
    require(settings.favorites.size() == 1, "one favorite should be present");
    require(!addFavorite(settings, L"Projects Again", L"d:\\projects"), "duplicate favorite should not be added case-insensitively");
    require(settings.favorites.size() == 1, "duplicate favorite should not change count");
    require(removeFavorite(settings, L"D:\\PROJECTS"), "favorite should be removed case-insensitively");
    require(settings.favorites.empty(), "favorite list should be empty after removal");
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

} // namespace

int main() {
    testDefaultsAndClamping();
    testFavoriteOperations();
    testSaveLoadRoundTrip();
    testMalformedSettingsFallback();
    std::cout << "AppSettingsTests passed\n";
}
```

- [ ] **Step 2: Add the test target and verify the red build**

Modify `CMakeLists.txt` after the `finderx_model` library block and before `finderx_fs`:

```cmake
add_library(finderx_settings
    src/settings/AppSettings.cpp
)

target_include_directories(finderx_settings PUBLIC src)
finderx_enable_utf8(finderx_settings)
finderx_enable_windows_unicode(finderx_settings)
```

Add this test block after `finderx_navigation_tests`:

```cmake
add_executable(finderx_settings_tests
    src/settings/AppSettingsTests.cpp
)

target_link_libraries(finderx_settings_tests PRIVATE finderx_settings)
finderx_enable_utf8(finderx_settings_tests)
finderx_enable_windows_unicode(finderx_settings_tests)
add_test(NAME finderx_settings_tests COMMAND finderx_settings_tests)
```

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_settings_tests
```

Expected: compile failure because `settings/AppSettings.h` does not exist yet.

- [ ] **Step 3: Add `AppSettings.h`**

Create `src/settings/AppSettings.h`:

```cpp
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
```

- [ ] **Step 4: Add `AppSettings.cpp`**

Create `src/settings/AppSettings.cpp`:

```cpp
#include "settings/AppSettings.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cwchar>
#include <fstream>
#include <sstream>

namespace finderx {
namespace {

std::wstring environmentVariable(const std::wstring& name) {
    const DWORD requiredLength = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
    if (requiredLength == 0) {
        return {};
    }

    std::wstring value(requiredLength, L'\0');
    const DWORD writtenLength = GetEnvironmentVariableW(name.c_str(), value.data(), requiredLength);
    if (writtenLength == 0 || writtenLength >= requiredLength) {
        return {};
    }

    value.resize(writtenLength);
    return value;
}

bool pathsEqual(const std::wstring& left, const std::wstring& right) {
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

std::wstring joinPath(const std::wstring& base, const std::wstring& child) {
    if (base.empty()) {
        return child;
    }
    const wchar_t last = base.back();
    if (last == L'\\' || last == L'/') {
        return base + child;
    }
    return base + L"\\" + child;
}

std::string wideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), required, nullptr, nullptr);
    return result;
}

std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int required = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), required);
    return result;
}

std::string escapeJsonString(const std::wstring& value) {
    std::string utf8 = wideToUtf8(value);
    std::string escaped;
    escaped.reserve(utf8.size());
    for (char ch : utf8) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

bool extractNumber(const std::string& text, const std::string& key, float& value) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPos = text.find(marker);
    if (keyPos == std::string::npos) {
        return false;
    }
    const std::size_t colon = text.find(':', keyPos + marker.size());
    if (colon == std::string::npos) {
        return false;
    }
    const std::size_t start = text.find_first_of("-0123456789", colon + 1);
    if (start == std::string::npos) {
        return false;
    }
    char* end = nullptr;
    value = std::strtof(text.c_str() + start, &end);
    return end != text.c_str() + start;
}

bool extractString(const std::string& text, const std::string& key, std::wstring& value) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPos = text.find(marker);
    if (keyPos == std::string::npos) {
        return false;
    }
    const std::size_t colon = text.find(':', keyPos + marker.size());
    if (colon == std::string::npos) {
        return false;
    }
    const std::size_t quote = text.find('"', colon + 1);
    if (quote == std::string::npos) {
        return false;
    }
    std::string raw;
    bool escaping = false;
    for (std::size_t index = quote + 1; index < text.size(); ++index) {
        const char ch = text[index];
        if (escaping) {
            switch (ch) {
            case 'n':
                raw.push_back('\n');
                break;
            case 'r':
                raw.push_back('\r');
                break;
            case 't':
                raw.push_back('\t');
                break;
            default:
                raw.push_back(ch);
                break;
            }
            escaping = false;
            continue;
        }
        if (ch == '\\') {
            escaping = true;
            continue;
        }
        if (ch == '"') {
            value = utf8ToWide(raw);
            return true;
        }
        raw.push_back(ch);
    }
    return false;
}

SortColumn parseSortColumn(const std::wstring& value) {
    if (value == L"modified") {
        return SortColumn::Modified;
    }
    if (value == L"size") {
        return SortColumn::Size;
    }
    if (value == L"kind") {
        return SortColumn::Kind;
    }
    return SortColumn::Name;
}

SortDirection parseSortDirection(const std::wstring& value) {
    return value == L"descending" ? SortDirection::Descending : SortDirection::Ascending;
}

std::vector<FavoriteLocation> parseFavorites(const std::string& text) {
    std::vector<FavoriteLocation> favorites;
    const std::size_t favoritesKey = text.find("\"favorites\"");
    if (favoritesKey == std::string::npos) {
        return favorites;
    }
    const std::size_t arrayStart = text.find('[', favoritesKey);
    const std::size_t arrayEnd = text.find(']', arrayStart);
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        return favorites;
    }

    std::size_t cursor = arrayStart;
    while (cursor < arrayEnd) {
        const std::size_t objectStart = text.find('{', cursor);
        if (objectStart == std::string::npos || objectStart >= arrayEnd) {
            break;
        }
        const std::size_t objectEnd = text.find('}', objectStart);
        if (objectEnd == std::string::npos || objectEnd > arrayEnd) {
            break;
        }
        const std::string object = text.substr(objectStart, objectEnd - objectStart + 1);
        FavoriteLocation favorite;
        if (extractString(object, "label", favorite.label) && extractString(object, "path", favorite.path) && !favorite.path.empty()) {
            favorites.push_back(std::move(favorite));
        }
        cursor = objectEnd + 1;
    }
    return favorites;
}

} // namespace

AppSettings makeDefaultSettings(const std::wstring& homePath) {
    AppSettings settings;
    settings.favorites.push_back({L"Home", homePath});
    settings.favorites.push_back({L"Desktop", joinPath(homePath, L"Desktop")});
    settings.favorites.push_back({L"Documents", joinPath(homePath, L"Documents")});
    settings.favorites.push_back({L"Downloads", joinPath(homePath, L"Downloads")});
    return settings;
}

void clampSettings(AppSettings& settings) {
    settings.fontSize = (std::clamp)(settings.fontSize, kMinFontSize, kMaxFontSize);
    settings.iconSize = (std::clamp)(settings.iconSize, kMinIconSize, kMaxIconSize);
}

bool containsFavorite(const AppSettings& settings, const std::wstring& path) {
    return std::any_of(settings.favorites.begin(), settings.favorites.end(), [&](const FavoriteLocation& favorite) {
        return pathsEqual(favorite.path, path);
    });
}

bool addFavorite(AppSettings& settings, std::wstring label, std::wstring path) {
    if (path.empty() || containsFavorite(settings, path)) {
        return false;
    }
    settings.favorites.push_back({std::move(label), std::move(path)});
    return true;
}

bool removeFavorite(AppSettings& settings, const std::wstring& path) {
    const auto oldEnd = settings.favorites.end();
    const auto newEnd = std::remove_if(settings.favorites.begin(), settings.favorites.end(), [&](const FavoriteLocation& favorite) {
        return pathsEqual(favorite.path, path);
    });
    const bool removed = newEnd != oldEnd;
    settings.favorites.erase(newEnd, oldEnd);
    return removed;
}

std::filesystem::path defaultSettingsPath() {
    const std::wstring appData = environmentVariable(L"APPDATA");
    if (!appData.empty()) {
        return std::filesystem::path(appData) / L"FinderX" / L"settings.json";
    }
    return std::filesystem::current_path() / L"settings.json";
}

SettingsLoadResult loadSettings(const std::wstring& homePath, const std::filesystem::path& path) {
    SettingsLoadResult result;
    result.settings = makeDefaultSettings(homePath);

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return result;
    }
    const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (text.find('{') == std::string::npos || text.find('}') == std::string::npos) {
        return result;
    }

    AppSettings loaded = result.settings;
    extractNumber(text, "fontSize", loaded.fontSize);
    extractNumber(text, "iconSize", loaded.iconSize);

    std::wstring column;
    if (extractString(text, "sortColumn", column)) {
        loaded.sortColumn = parseSortColumn(column);
    }

    std::wstring direction;
    if (extractString(text, "sortDirection", direction)) {
        loaded.sortDirection = parseSortDirection(direction);
    }

    std::vector<FavoriteLocation> favorites = parseFavorites(text);
    if (!favorites.empty()) {
        loaded.favorites = std::move(favorites);
    }

    clampSettings(loaded);
    result.settings = std::move(loaded);
    result.loadedFromDisk = true;
    return result;
}

bool saveSettings(const AppSettings& settings, const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
        return false;
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return false;
    }

    stream << "{\n";
    stream << "  \"fontSize\": " << settings.fontSize << ",\n";
    stream << "  \"iconSize\": " << settings.iconSize << ",\n";
    stream << "  \"sortColumn\": \"" << escapeJsonString(sortColumnName(settings.sortColumn)) << "\",\n";
    stream << "  \"sortDirection\": \"" << escapeJsonString(sortDirectionName(settings.sortDirection)) << "\",\n";
    stream << "  \"favorites\": [\n";
    for (std::size_t index = 0; index < settings.favorites.size(); ++index) {
        const FavoriteLocation& favorite = settings.favorites[index];
        stream << "    { \"label\": \"" << escapeJsonString(favorite.label) << "\", \"path\": \"" << escapeJsonString(favorite.path) << "\" }";
        if (index + 1 < settings.favorites.size()) {
            stream << ",";
        }
        stream << "\n";
    }
    stream << "  ]\n";
    stream << "}\n";
    return static_cast<bool>(stream);
}

std::wstring sortColumnName(SortColumn column) {
    switch (column) {
    case SortColumn::Modified:
        return L"modified";
    case SortColumn::Size:
        return L"size";
    case SortColumn::Kind:
        return L"kind";
    case SortColumn::Name:
    default:
        return L"name";
    }
}

std::wstring sortDirectionName(SortDirection direction) {
    return direction == SortDirection::Descending ? L"descending" : L"ascending";
}

} // namespace finderx
```

- [ ] **Step 5: Run settings tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_settings_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_settings_tests --output-on-failure
```

Expected: `finderx_settings_tests` passes.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/settings/AppSettings.h src/settings/AppSettings.cpp src/settings/AppSettingsTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add app settings persistence"
```

---

### Task 2: File Metadata And Sorter

**Files:**
- Create: `src/fs/FileSorter.h`
- Create: `src/fs/FileSorter.cpp`
- Create: `src/fs/FileSorterTests.cpp`
- Modify: `src/model/FileNode.h`
- Modify: `src/fs/FileMetadata.h`
- Modify: `src/fs/DirectoryLoader.cpp`
- Modify: `src/fs/DirectoryLoaderTests.cpp`
- Modify: `src/fs/DriveEnumerator.cpp`
- Modify: `src/fs/DriveEnumeratorTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add failing sorter tests**

Create `src/fs/FileSorterTests.cpp`:

```cpp
#include "fs/FileSorter.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

FileNode node(std::wstring name, FileKind kind, unsigned long long modifiedTicks, unsigned long long sizeBytes, std::wstring kindText) {
    FileNode item;
    item.name = std::move(name);
    item.path = item.name;
    item.kind = kind;
    item.modifiedTicks = modifiedTicks;
    item.sizeBytes = sizeBytes;
    item.kindText = std::move(kindText);
    return item;
}

void testNameSortKeepsFoldersFirst() {
    std::vector<FileNode> nodes{
        node(L"z-file.txt", FileKind::File, 1, 10, L"Text"),
        node(L"b-folder", FileKind::Folder, 1, 0, L"Folder"),
        node(L"a-file.txt", FileKind::File, 1, 20, L"Text"),
        node(L"a-folder", FileKind::Folder, 1, 0, L"Folder"),
    };

    sortFileNodes(nodes, SortColumn::Name, SortDirection::Ascending);
    require(nodes[0].name == L"a-folder", "folders should sort first by name");
    require(nodes[1].name == L"b-folder", "second folder should sort by name");
    require(nodes[2].name == L"a-file.txt", "files should sort after folders by name");
    require(nodes[3].name == L"z-file.txt", "last file should sort by name");

    sortFileNodes(nodes, SortColumn::Name, SortDirection::Descending);
    require(nodes[0].name == L"b-folder", "descending folders should stay before files");
    require(nodes[2].name == L"z-file.txt", "descending files should sort by name");
}

void testModifiedSort() {
    std::vector<FileNode> nodes{
        node(L"old.txt", FileKind::File, 10, 1, L"Text"),
        node(L"new.txt", FileKind::File, 30, 1, L"Text"),
        node(L"mid.txt", FileKind::File, 20, 1, L"Text"),
    };

    sortFileNodes(nodes, SortColumn::Modified, SortDirection::Ascending);
    require(nodes[0].name == L"old.txt", "oldest modified should be first ascending");
    require(nodes[2].name == L"new.txt", "newest modified should be last ascending");

    sortFileNodes(nodes, SortColumn::Modified, SortDirection::Descending);
    require(nodes[0].name == L"new.txt", "newest modified should be first descending");
    require(nodes[2].name == L"old.txt", "oldest modified should be last descending");
}

void testSizeAndKindSort() {
    std::vector<FileNode> nodes{
        node(L"b.md", FileKind::File, 1, 20, L"Markdown"),
        node(L"a.txt", FileKind::File, 1, 10, L"Text"),
        node(L"c.bin", FileKind::File, 1, 30, L"Binary"),
    };

    sortFileNodes(nodes, SortColumn::Size, SortDirection::Ascending);
    require(nodes[0].name == L"a.txt", "smallest file should be first");
    require(nodes[2].name == L"c.bin", "largest file should be last");

    sortFileNodes(nodes, SortColumn::Kind, SortDirection::Ascending);
    require(nodes[0].name == L"c.bin", "Binary kind should sort first");
    require(nodes[1].name == L"b.md", "Markdown kind should sort second");
    require(nodes[2].name == L"a.txt", "Text kind should sort third");
}

} // namespace

int main() {
    testNameSortKeepsFoldersFirst();
    testModifiedSort();
    testSizeAndKindSort();
    std::cout << "FileSorterTests passed\n";
}
```

- [ ] **Step 2: Register the sorter target and verify red build**

Add `src/fs/FileSorter.cpp` to `finderx_fs`:

```cmake
add_library(finderx_fs
    src/fs/FileMetadata.cpp
    src/fs/DirectoryLoader.cpp
    src/fs/FileCreation.cpp
    src/fs/DriveEnumerator.cpp
    src/fs/FileSorter.cpp
)
```

Update the `finderx_fs` link line because `FileSorter.h` uses `SortColumn` and `SortDirection` from `finderx_settings`:

```cmake
target_link_libraries(finderx_fs PUBLIC finderx_model finderx_settings)
```

Add test block after drive enumerator tests:

```cmake
add_executable(finderx_file_sorter_tests
    src/fs/FileSorterTests.cpp
)

target_link_libraries(finderx_file_sorter_tests PRIVATE finderx_fs finderx_settings)
finderx_enable_utf8(finderx_file_sorter_tests)
finderx_enable_windows_unicode(finderx_file_sorter_tests)
add_test(NAME finderx_file_sorter_tests COMMAND finderx_file_sorter_tests)
```

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_file_sorter_tests
```

Expected: compile failure because `fs/FileSorter.h` and raw metadata fields do not exist.

- [ ] **Step 3: Add sortable metadata to nodes and loader**

Modify `src/model/FileNode.h` by adding fields after `kindText`:

```cpp
    unsigned long long modifiedTicks = 0;
    unsigned long long sizeBytes = 0;
```

Modify `src/fs/FileMetadata.h` by adding to `FileMetadata` after `kindText`:

```cpp
    unsigned long long modifiedTicks = 0;
    unsigned long long sizeBytes = 0;
```

In `src/fs/DirectoryLoader.cpp`, after the line that assigns `node.kindText = kindTextForAttributes(data.dwFileAttributes);`, set:

```cpp
        node.modifiedTicks = (static_cast<unsigned long long>(data.ftLastWriteTime.dwHighDateTime) << 32)
            | static_cast<unsigned long long>(data.ftLastWriteTime.dwLowDateTime);
        node.sizeBytes = isDirectory ? 0 : fileSizeFromFindData(data);
```

Keep `node.size = formatFileSize(fileSizeFromFindData(data), isDirectory);` unchanged unless you store the file size in a local variable first:

```cpp
        const unsigned long long sizeBytes = fileSizeFromFindData(data);
        node.size = formatFileSize(sizeBytes, isDirectory);
        node.sizeBytes = isDirectory ? 0 : sizeBytes;
```

In `src/fs/DriveEnumerator.cpp`, set drive nodes:

```cpp
    node.modifiedTicks = 0;
    node.sizeBytes = 0;
```

- [ ] **Step 4: Add sorter implementation**

Create `src/fs/FileSorter.h`:

```cpp
#pragma once

#include "model/FileNode.h"
#include "settings/AppSettings.h"

#include <vector>

namespace finderx {

void sortFileNodes(std::vector<FileNode>& nodes, SortColumn column, SortDirection direction);

} // namespace finderx
```

Create `src/fs/FileSorter.cpp`:

```cpp
#include "fs/FileSorter.h"

#include <cwchar>
#include <tuple>

namespace finderx {
namespace {

int compareText(const std::wstring& left, const std::wstring& right) {
    return _wcsicmp(left.c_str(), right.c_str());
}

template <typename T>
int compareValue(const T& left, const T& right) {
    if (left < right) {
        return -1;
    }
    if (right < left) {
        return 1;
    }
    return 0;
}

int compareByColumn(const FileNode& left, const FileNode& right, SortColumn column) {
    switch (column) {
    case SortColumn::Modified:
        return compareValue(left.modifiedTicks, right.modifiedTicks);
    case SortColumn::Size:
        return compareValue(left.sizeBytes, right.sizeBytes);
    case SortColumn::Kind:
        return compareText(left.kindText, right.kindText);
    case SortColumn::Name:
    default:
        return compareText(left.name, right.name);
    }
}

} // namespace

void sortFileNodes(std::vector<FileNode>& nodes, SortColumn column, SortDirection direction) {
    std::stable_sort(nodes.begin(), nodes.end(), [&](const FileNode& left, const FileNode& right) {
        if (left.kind != right.kind) {
            return left.kind == FileKind::Folder;
        }

        int comparison = compareByColumn(left, right, column);
        if (direction == SortDirection::Descending) {
            comparison = -comparison;
        }

        if (comparison != 0) {
            return comparison < 0;
        }

        return compareText(left.name, right.name) < 0;
    });
}

} // namespace finderx
```

Add `#include <algorithm>` to `src/fs/FileSorter.cpp` if the compiler reports `std::stable_sort` missing.

- [ ] **Step 5: Update metadata tests**

In `src/fs/DirectoryLoaderTests.cpp`, add assertions after existing loaded-node metadata assertions:

```cpp
    require(loaded[0].modifiedTicks > 0, "loaded folder should have raw modified ticks");
    require(loaded[2].sizeBytes > 0, "loaded file should have raw size bytes");
```

In `src/fs/DriveEnumeratorTests.cpp`, add:

```cpp
    require(nodes[0].modifiedTicks == 0, "drive node modified ticks should be zero");
    require(nodes[0].sizeBytes == 0, "drive node size bytes should be zero");
```

- [ ] **Step 6: Run sorter and filesystem tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_file_sorter_tests finderx_fs_tests finderx_drive_enumerator_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R "finderx_file_sorter_tests|finderx_fs_tests|finderx_drive_enumerator_tests" --output-on-failure
```

Expected: all three tests pass.

- [ ] **Step 7: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/fs/FileSorter.h src/fs/FileSorter.cpp src/fs/FileSorterTests.cpp src/model/FileNode.h src/fs/FileMetadata.h src/fs/DirectoryLoader.cpp src/fs/DirectoryLoaderTests.cpp src/fs/DriveEnumerator.cpp src/fs/DriveEnumeratorTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add file sorting metadata"
```

---

### Task 3: Apply Sort Settings And Header/Toolbar Sorting

**Files:**
- Modify: `src/ui/FinderChrome.h`
- Modify: `src/ui/FinderChrome.cpp`
- Modify: `src/navigation/SidebarModel.h`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Extend chrome state and hit results**

Modify `src/navigation/SidebarModel.h` by including settings:

```cpp
#include "settings/AppSettings.h"
```

Add fields to `ChromeState`:

```cpp
    SortColumn sortColumn = SortColumn::Name;
    SortDirection sortDirection = SortDirection::Ascending;
```

Modify `src/ui/FinderChrome.h`:

```cpp
enum class ChromeHitKind {
    None,
    Back,
    Forward,
    SearchField,
    SidebarItem,
    Tab,
    NewTab,
    SortMenu,
    Settings,
    HeaderName,
    HeaderModified,
    HeaderSize,
    HeaderKind
};
```

- [ ] **Step 2: Draw toolbar Sort/Settings buttons and header arrows**

In `src/ui/FinderChrome.cpp`, add small button helpers near other anonymous namespace helpers:

```cpp
D2D1_RECT_F sortButtonRect(const D2D1_RECT_F& toolbar) {
    return D2D1::RectF(toolbar.right - 360.0f, toolbar.top + 13.0f, toolbar.right - 300.0f, toolbar.top + 43.0f);
}

D2D1_RECT_F settingsButtonRect(const D2D1_RECT_F& toolbar) {
    return D2D1::RectF(toolbar.right - 292.0f, toolbar.top + 13.0f, toolbar.right - 214.0f, toolbar.top + 43.0f);
}

std::wstring sortArrow(const ChromeState& state, SortColumn column) {
    if (state.sortColumn != column) {
        return L"";
    }
    return state.sortDirection == SortDirection::Ascending ? L" \u2191" : L" \u2193";
}
```

In `FinderChrome::draw`, before the search field, draw:

```cpp
    const D2D1_RECT_F sortRect = sortButtonRect(rects.toolbar);
    if (sortRect.left < sortRect.right) {
        render.fillRoundedRect(D2D1::RoundedRect(sortRect, 7.0f, 7.0f), D2D1::ColorF(0.90f, 0.91f, 0.92f));
        drawTextClipped(render, L"Sort", D2D1::RectF(sortRect.left + 10.0f, sortRect.top + 6.0f, sortRect.right - 8.0f, sortRect.bottom), rects.toolbar, render.textFormat(), D2D1::ColorF(0.25f, 0.25f, 0.25f));
    }

    const D2D1_RECT_F settingsRect = settingsButtonRect(rects.toolbar);
    if (settingsRect.left < settingsRect.right) {
        render.fillRoundedRect(D2D1::RoundedRect(settingsRect, 7.0f, 7.0f), D2D1::ColorF(0.90f, 0.91f, 0.92f));
        drawTextClipped(render, L"Settings", D2D1::RectF(settingsRect.left + 10.0f, settingsRect.top + 6.0f, settingsRect.right - 8.0f, settingsRect.bottom), rects.toolbar, render.textFormat(), D2D1::ColorF(0.25f, 0.25f, 0.25f));
    }
```

Update header label draws to append arrows. Use local `std::wstring` variables for each label:

```cpp
        const std::wstring nameLabel = std::wstring(L"Name") + sortArrow(state, SortColumn::Name);
        drawTextClipped(render, nameLabel, D2D1::RectF(rects.header.left + 64.0f, labelTop, nameRight, labelBottom), rects.header, render.headerTextFormat(), D2D1::ColorF(0.18f, 0.18f, 0.18f));
```

Repeat for `Date Modified`, `Size`, and `Kind`.

- [ ] **Step 3: Hit-test sort/settings/header**

In `FinderChrome::hitTest`, after forward button and before search:

```cpp
    const D2D1_RECT_F sortRect = sortButtonRect(rects.toolbar);
    if (containsPoint(sortRect, x, y)) {
        return {ChromeHitKind::SortMenu, 0, 0};
    }

    const D2D1_RECT_F settingsRect = settingsButtonRect(rects.toolbar);
    if (containsPoint(settingsRect, x, y)) {
        return {ChromeHitKind::Settings, 0, 0};
    }
```

Add header hit testing using the same column geometry as drawing. If a column is hidden at the current width, do not return its hit kind:

```cpp
    if (containsPoint(rects.header, x, y)) {
        const float headerWidth = rects.header.right - rects.header.left;
        const float padding = 12.0f;
        const float dateWidth = headerWidth >= 520.0f ? 150.0f : 0.0f;
        const float sizeWidth = headerWidth >= 420.0f ? 80.0f : 0.0f;
        const float kindWidth = headerWidth >= 360.0f ? 120.0f : 0.0f;
        const float kindX = rects.header.right - padding - kindWidth;
        const float sizeX = kindX - sizeWidth;
        const float dateX = sizeX - dateWidth;
        const float trailingLeft = dateWidth > 0.0f
            ? dateX
            : (sizeWidth > 0.0f ? sizeX : (kindWidth > 0.0f ? kindX : rects.header.right - padding));
        const float nameRight = trailingLeft - 8.0f;
        if (x >= rects.header.left + 64.0f && x <= nameRight) {
            return {ChromeHitKind::HeaderName, 0, 0};
        }
        if (dateWidth > 0.0f && x >= dateX && x <= sizeX - 8.0f) {
            return {ChromeHitKind::HeaderModified, 0, 0};
        }
        if (sizeWidth > 0.0f && x >= sizeX && x <= kindX - 8.0f) {
            return {ChromeHitKind::HeaderSize, 0, 0};
        }
        if (kindWidth > 0.0f && x >= kindX && x <= rects.header.right - padding) {
            return {ChromeHitKind::HeaderKind, 0, 0};
        }
    }
```

- [ ] **Step 4: Wire sort state into MainWindow**

Modify `src/ui/MainWindow.h` includes:

```cpp
#include "settings/AppSettings.h"
```

Add members:

```cpp
    AppSettings settings_;
```

Add methods:

```cpp
    void applySort(std::vector<FileNode>& nodes) const;
    void changeSort(SortColumn column);
    void showSortMenu(POINT screenPoint);
    bool saveSettingsOrStatus();
```

In `src/ui/MainWindow.cpp`, include:

```cpp
#include "fs/FileSorter.h"
#include "settings/AppSettings.h"
```

In `initializeFileTree`, after `homePath_ = defaultHomeDirectory();`:

```cpp
    settings_ = loadSettings(homePath_).settings;
```

Add helper:

```cpp
void MainWindow::applySort(std::vector<FileNode>& nodes) const {
    sortFileNodes(nodes, settings_.sortColumn, settings_.sortDirection);
}
```

Call `applySort(result.children);` before every `replaceChildren` for real directory loads:

- `createTabAtPath`
- `navigateToDirectory`
- `refreshCurrentDirectorySelecting`
- `loadChildrenIfNeeded`

Do not sort `enumerateDriveNodes()` for `This PC`.

In `refreshChromeState`, set:

```cpp
    chromeState_.sortColumn = settings_.sortColumn;
    chromeState_.sortDirection = settings_.sortDirection;
```

- [ ] **Step 5: Add sort actions and menu**

Add command IDs in `MainWindow.cpp`:

```cpp
constexpr UINT kCommandSortName = 1013;
constexpr UINT kCommandSortModified = 1014;
constexpr UINT kCommandSortSize = 1015;
constexpr UINT kCommandSortKind = 1016;
constexpr UINT kCommandSortAscending = 1017;
constexpr UINT kCommandSortDescending = 1018;
```

Add `showSortMenu`:

```cpp
void MainWindow::showSortMenu(POINT screenPoint) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kCommandSortName, L"Name");
    AppendMenuW(menu, MF_STRING, kCommandSortModified, L"Modified");
    AppendMenuW(menu, MF_STRING, kCommandSortSize, L"Size");
    AppendMenuW(menu, MF_STRING, kCommandSortKind, L"Kind");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kCommandSortAscending, L"Ascending");
    AppendMenuW(menu, MF_STRING, kCommandSortDescending, L"Descending");

    CheckMenuItem(menu, kCommandSortName, MF_BYCOMMAND | (settings_.sortColumn == SortColumn::Name ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, kCommandSortModified, MF_BYCOMMAND | (settings_.sortColumn == SortColumn::Modified ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, kCommandSortSize, MF_BYCOMMAND | (settings_.sortColumn == SortColumn::Size ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, kCommandSortKind, MF_BYCOMMAND | (settings_.sortColumn == SortColumn::Kind ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, kCommandSortAscending, MF_BYCOMMAND | (settings_.sortDirection == SortDirection::Ascending ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, kCommandSortDescending, MF_BYCOMMAND | (settings_.sortDirection == SortDirection::Descending ? MF_CHECKED : MF_UNCHECKED));

    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}
```

Add `changeSort`:

```cpp
void MainWindow::changeSort(SortColumn column) {
    if (settings_.sortColumn == column) {
        settings_.sortDirection = settings_.sortDirection == SortDirection::Ascending
            ? SortDirection::Descending
            : SortDirection::Ascending;
    } else {
        settings_.sortColumn = column;
        settings_.sortDirection = SortDirection::Ascending;
    }

    saveSettingsOrStatus();
    refreshCurrentDirectory();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

Add `saveSettingsOrStatus`:

```cpp
bool MainWindow::saveSettingsOrStatus() {
    if (!saveSettings(settings_)) {
        setStatusText(L"Cannot save settings");
        return false;
    }
    return true;
}
```

Handle chrome hits in `WM_LBUTTONDOWN`:

```cpp
        case ChromeHitKind::SortMenu:
            ClientToScreen(hwnd_, &clientPoint);
            showSortMenu(clientPoint);
            return 0;
        case ChromeHitKind::Settings:
            return 0;
        case ChromeHitKind::HeaderName:
            changeSort(SortColumn::Name);
            return 0;
        case ChromeHitKind::HeaderModified:
            changeSort(SortColumn::Modified);
            return 0;
        case ChromeHitKind::HeaderSize:
            changeSort(SortColumn::Size);
            return 0;
        case ChromeHitKind::HeaderKind:
            changeSort(SortColumn::Kind);
            return 0;
```

The Settings hit is intentionally a no-op until Task 6 adds the dialog.

Handle commands:

```cpp
    case kCommandSortName:
        changeSort(SortColumn::Name);
        break;
    case kCommandSortModified:
        changeSort(SortColumn::Modified);
        break;
    case kCommandSortSize:
        changeSort(SortColumn::Size);
        break;
    case kCommandSortKind:
        changeSort(SortColumn::Kind);
        break;
    case kCommandSortAscending:
        settings_.sortDirection = SortDirection::Ascending;
        saveSettingsOrStatus();
        refreshCurrentDirectory();
        break;
    case kCommandSortDescending:
        settings_.sortDirection = SortDirection::Descending;
        saveSettingsOrStatus();
        refreshCurrentDirectory();
        break;
```

- [ ] **Step 6: Link settings and sorter into app**

Modify `FinderX` target links in `CMakeLists.txt`:

```cmake
target_link_libraries(FinderX PRIVATE
    finderx_model
    finderx_fs
    finderx_navigation
    finderx_settings
    d2d1
    dwrite
    windowscodecs
    ole32
    shell32
)
```

- [ ] **Step 7: Build and run focused tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_file_sorter_tests finderx_settings_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R "finderx_file_sorter_tests|finderx_settings_tests" --output-on-failure
```

Expected: build succeeds and focused tests pass.

- [ ] **Step 8: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/ui/FinderChrome.h src/ui/FinderChrome.cpp src/navigation/SidebarModel.h src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add global file sorting UI"
```

---

### Task 4: Favorites Sidebar And Context Actions

**Files:**
- Modify: `src/navigation/SidebarModel.h`
- Modify: `src/navigation/SidebarModel.cpp`
- Modify: `src/navigation/NavigationHistoryTests.cpp`
- Modify: `src/ui/FinderChrome.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add sidebar role and grouping tests**

Modify `src/navigation/NavigationHistoryTests.cpp` in `testSidebarModel()` to use favorites:

```cpp
    std::vector<FavoriteLocation> favorites{
        {L"Home", L"C:\\Users\\Example"},
        {L"Desktop", L"C:\\Users\\Example\\Desktop"},
        {L"Projects", L"D:\\Projects"},
    };
    model.refresh(L"C:\\Users\\Example", L"D:\\Projects", favorites);
    require(findItem(model, L"Home").role == SidebarItemRole::Favorite, "Home should be a favorite");
    require(findItem(model, L"Projects").selected, "current favorite should be selected");
    require(findItem(model, L"This PC").role == SidebarItemRole::Location, "This PC should be a location");
```

Keep existing `This PC` tests, but update the `refresh` calls to pass favorites:

```cpp
model.refresh(L"C:\\Users\\Example", thisPcPath(), favorites);
```

- [ ] **Step 2: Extend SidebarModel**

In `src/navigation/SidebarModel.h`, add:

```cpp
#include "settings/AppSettings.h"
```

Add enum:

```cpp
enum class SidebarItemRole {
    Favorite,
    Location
};
```

Add field:

```cpp
    SidebarItemRole role = SidebarItemRole::Location;
```

Change refresh signature:

```cpp
void refresh(const std::wstring& homePath, const std::wstring& currentPath, const std::vector<FavoriteLocation>& favorites);
```

In `src/navigation/SidebarModel.cpp`, add helper:

```cpp
SidebarItem makeFavoriteItem(const FavoriteLocation& favorite, const std::wstring& currentPath) {
    SidebarItem item;
    item.label = favorite.label;
    item.path = favorite.path;
    item.available = directoryExists(favorite.path);
    item.selected = item.available && pathsEqual(favorite.path, currentPath);
    item.role = SidebarItemRole::Favorite;
    return item;
}
```

Set role in `makeThisPcItem`:

```cpp
    item.role = SidebarItemRole::Location;
```

Set role in `makeItem`:

```cpp
    item.role = SidebarItemRole::Location;
```

Replace `SidebarModel::refresh` body:

```cpp
void SidebarModel::refresh(const std::wstring&, const std::wstring& currentPath, const std::vector<FavoriteLocation>& favorites) {
    items_.clear();
    items_.reserve(favorites.size() + 2);
    for (const FavoriteLocation& favorite : favorites) {
        items_.push_back(makeFavoriteItem(favorite, currentPath));
    }
    items_.push_back(makeThisPcItem(currentPath));
    items_.push_back(makeItem(L"Applications", userProgramsDirectory(), currentPath));
}
```

- [ ] **Step 3: Draw grouped sidebar headers**

In `FinderChrome::draw`, replace the single `Favorites` header with two dynamic sections:

```cpp
    bool drewFavoritesHeader = false;
    bool drewLocationsHeader = false;
    float rowY = kSidebarRowStartY;
    for (std::size_t index = 0; index < state.sidebarItems.size(); ++index) {
        const SidebarItem& item = state.sidebarItems[index];
        if (item.role == SidebarItemRole::Favorite && !drewFavoritesHeader) {
            drawTextClipped(render, L"Favorites", D2D1::RectF(18.0f, rowY - 24.0f, 150.0f, rowY - 4.0f), rects.sidebar, render.headerTextFormat(), D2D1::ColorF(0.55f, 0.55f, 0.55f));
            drewFavoritesHeader = true;
        }
        if (item.role == SidebarItemRole::Location && !drewLocationsHeader) {
            rowY += drewFavoritesHeader ? 16.0f : 0.0f;
            drawTextClipped(render, L"Locations", D2D1::RectF(18.0f, rowY - 24.0f, 150.0f, rowY - 4.0f), rects.sidebar, render.headerTextFormat(), D2D1::ColorF(0.55f, 0.55f, 0.55f));
            drewLocationsHeader = true;
        }

        const float itemY = rowY;
        // use itemY for existing row drawing
        rowY += kSidebarRowStep;
    }
```

Make the same row-position logic in `FinderChrome::hitTest`. Use a helper function if duplication becomes error-prone:

```cpp
float sidebarRowYForIndex(const ChromeState& state, std::size_t targetIndex) {
    bool sawFavorites = false;
    bool sawLocations = false;
    float rowY = kSidebarRowStartY;
    for (std::size_t index = 0; index <= targetIndex && index < state.sidebarItems.size(); ++index) {
        const SidebarItem& item = state.sidebarItems[index];
        if (item.role == SidebarItemRole::Favorite && !sawFavorites) {
            sawFavorites = true;
        }
        if (item.role == SidebarItemRole::Location && !sawLocations) {
            rowY += sawFavorites ? 16.0f : 0.0f;
            sawLocations = true;
        }
        if (index == targetIndex) {
            return rowY;
        }
        rowY += kSidebarRowStep;
    }
    return rowY;
}
```

- [ ] **Step 4: Update MainWindow sidebar refresh**

Every call to `sidebar_.refresh` becomes:

```cpp
sidebar_.refresh(homePath_, currentPath, settings_.favorites);
```

In `refreshChromeState`, pass the current tab path or empty path:

```cpp
sidebar_.refresh(homePath_, tab.history.currentPath(), settings_.favorites);
```

- [ ] **Step 5: Add add/remove favorite commands**

Add command IDs:

```cpp
constexpr UINT kCommandAddFavorite = 1019;
constexpr UINT kCommandRemoveFavorite = 1020;
```

Add members to `MainWindow.h`:

```cpp
    std::size_t contextSidebarIndex_ = static_cast<std::size_t>(-1);
    void showSidebarContextMenu(std::size_t sidebarIndex, POINT screenPoint);
    void addCurrentDirectoryToFavorites();
    void removeContextFavorite();
```

In `WM_RBUTTONDOWN`, before list context menu, hit-test chrome:

```cpp
        const LayoutRects rects = currentLayout();
        const ChromeHitResult chromeHit = chrome_.hitTest(dipPoint.x, dipPoint.y, rects, chromeState_);
        if (chromeHit.kind == ChromeHitKind::SidebarItem) {
            showSidebarContextMenu(chromeHit.sidebarIndex, screenPoint);
            return 0;
        }
```

Add to list background context menu when `directoryLocation`:

```cpp
        AppendMenuW(menu, MF_STRING, kCommandAddFavorite, L"Add to Favorites");
```

Implement:

```cpp
void MainWindow::showSidebarContextMenu(std::size_t sidebarIndex, POINT screenPoint) {
    contextSidebarIndex_ = sidebarIndex;
    if (sidebarIndex >= chromeState_.sidebarItems.size()) {
        return;
    }
    const SidebarItem& item = chromeState_.sidebarItems[sidebarIndex];
    if (item.role != SidebarItemRole::Favorite) {
        return;
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    AppendMenuW(menu, MF_STRING, kCommandRemoveFavorite, L"Remove from Favorites");
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

void MainWindow::addCurrentDirectoryToFavorites() {
    if (!isActiveDirectoryLocation()) {
        return;
    }
    const std::wstring path = activeTab().history.currentPath();
    if (path.empty()) {
        setStatusText(L"Cannot add favorite");
        return;
    }
    std::wstring label = fileNameFromPath(path);
    if (label.empty()) {
        label = path;
    }
    if (addFavorite(settings_, label, path)) {
        saveSettingsOrStatus();
        refreshChromeState();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void MainWindow::removeContextFavorite() {
    if (contextSidebarIndex_ >= chromeState_.sidebarItems.size()) {
        return;
    }
    const SidebarItem item = chromeState_.sidebarItems[contextSidebarIndex_];
    if (item.role != SidebarItemRole::Favorite) {
        return;
    }
    if (removeFavorite(settings_, item.path)) {
        saveSettingsOrStatus();
        refreshChromeState();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}
```

Handle commands:

```cpp
    case kCommandAddFavorite:
        addCurrentDirectoryToFavorites();
        break;
    case kCommandRemoveFavorite:
        removeContextFavorite();
        break;
```

- [ ] **Step 6: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_navigation_tests finderx_settings_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R "finderx_navigation_tests|finderx_settings_tests" --output-on-failure
```

Expected: build succeeds and tests pass.

- [ ] **Step 7: Commit**

```powershell
git -c safe.directory=D:/finderX add src/navigation/SidebarModel.h src/navigation/SidebarModel.cpp src/navigation/NavigationHistoryTests.cpp src/ui/FinderChrome.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add favorites sidebar"
```

---

### Task 5: Configurable List Style And Render Font Size

**Files:**
- Modify: `src/ui/RenderContext.h`
- Modify: `src/ui/RenderContext.cpp`
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`
- Modify: `src/ui/FinderListViewTests.cpp`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add style tests for list hit testing**

In `src/ui/FinderListViewTests.cpp`, add:

```cpp
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        ListViewStyle style;
        style.fontSize = 18.0f;
        style.iconSize = 24.0f;
        view.setStyle(style);
        const D2D1_RECT_F bounds = D2D1::RectF(0.0f, 0.0f, 900.0f, 600.0f);
        const NodeId first = view.nodeAtPoint(70.0f, 7.0f, bounds);
        require(first != kInvalidNodeId, "large style should keep first row hit-testable");
        const ListInteractionResult result = view.onMouseDown(70.0f, 7.0f, bounds, false, false);
        require(result.changed || view.hasSelection(), "large style click should produce or keep selection");
```

- [ ] **Step 2: Add ListViewStyle API**

Modify `src/ui/FinderListView.h`:

```cpp
struct ListViewStyle {
    float fontSize = 13.0f;
    float iconSize = 14.0f;
};
```

Add public methods:

```cpp
    void setStyle(ListViewStyle style);
    ListViewStyle style() const;
```

Add private helpers and member:

```cpp
    float rowHeight() const;
    float iconSize() const;
    ListViewStyle style_;
```

In `FinderListView.cpp`, replace fixed uses:

```cpp
float FinderListView::rowHeight() const {
    return (std::max)({20.0f, style_.iconSize + 6.0f, style_.fontSize + 8.0f});
}

float FinderListView::iconSize() const {
    return (std::clamp)(style_.iconSize, 12.0f, 24.0f);
}

void FinderListView::setStyle(ListViewStyle style) {
    style.fontSize = (std::clamp)(style.fontSize, 11.0f, 18.0f);
    style.iconSize = (std::clamp)(style.iconSize, 12.0f, 24.0f);
    style_ = style;
    clampScroll();
}

ListViewStyle FinderListView::style() const {
    return style_;
}
```

Replace calculations that use `kRowHeight` with `rowHeight()`. Replace icon dimensions with `iconSize()` where icons are drawn and text offset is calculated. Keep `kDisclosureWidth` stable.

- [ ] **Step 3: Add RenderContext font size support**

Modify `RenderContext.h`:

```cpp
    bool setFontSize(float fontSize);
    float fontSize() const;
```

Add member:

```cpp
    float fontSize_ = 13.0f;
```

In `RenderContext.cpp`, create a helper:

```cpp
bool RenderContext::setFontSize(float fontSize) {
    fontSize = (std::clamp)(fontSize, 11.0f, 18.0f);
    if (fontSize_ == fontSize && textFormat_ && headerTextFormat_) {
        return true;
    }
    fontSize_ = fontSize;
    textFormat_.Reset();
    headerTextFormat_.Reset();
    return createFactories();
}

float RenderContext::fontSize() const {
    return fontSize_;
}
```

Update `createFactories()` to use `fontSize_` for text format and `(std::max)(11.0f, fontSize_ - 1.0f)` for header format.

Add `#include <algorithm>` to `RenderContext.cpp`.

- [ ] **Step 4: Apply style in MainWindow**

Add helper in `MainWindow.h`:

```cpp
    ListViewStyle currentListViewStyle() const;
    void applyListStyle(TabState& tab) const;
    void applyVisualSettings();
```

Implement:

```cpp
ListViewStyle MainWindow::currentListViewStyle() const {
    ListViewStyle style;
    style.fontSize = settings_.fontSize;
    style.iconSize = settings_.iconSize;
    return style;
}

void MainWindow::applyListStyle(TabState& tab) const {
    tab.listView.setStyle(currentListViewStyle());
}

void MainWindow::applyVisualSettings() {
    render_.setFontSize(settings_.fontSize);
    for (const std::unique_ptr<TabState>& tab : tabs_) {
        if (tab) {
            tab->listView.setStyle(currentListViewStyle());
        }
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

After every `tab.listView = FinderListView(&tab.tree);`, call:

```cpp
    applyListStyle(tab);
```

After loading settings in `initializeFileTree`, call:

```cpp
    applyVisualSettings();
```

- [ ] **Step 5: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_list_view_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_list_view_tests --output-on-failure
```

Expected: build succeeds and list view tests pass.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/RenderContext.h src/ui/RenderContext.cpp src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add configurable list style"
```

---

### Task 6: Settings Dialog Integration

**Files:**
- Create: `src/ui/SettingsDialog.h`
- Create: `src/ui/SettingsDialog.cpp`
- Create: `src/ui/SettingsDialogTests.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add settings dialog helper tests**

Create `src/ui/SettingsDialogTests.cpp`:

```cpp
#include "ui/SettingsDialog.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    SettingsDialogValues values;
    values.fontSizeText = L"16";
    values.iconSizeText = L"20";
    AppSettings settings;
    settings.fontSize = 13.0f;
    settings.iconSize = 14.0f;
    applySettingsDialogValues(values, settings);
    require(settings.fontSize == 16.0f, "font size text should apply");
    require(settings.iconSize == 20.0f, "icon size text should apply");

    values.fontSizeText = L"abc";
    values.iconSizeText = L"99";
    applySettingsDialogValues(values, settings);
    require(settings.fontSize == 16.0f, "invalid font text should keep previous value");
    require(settings.iconSize == kMaxIconSize, "icon size should clamp to max");

    std::cout << "SettingsDialogTests passed\n";
}
```

- [ ] **Step 2: Add dialog API**

Create `src/ui/SettingsDialog.h`:

```cpp
#pragma once

#include "settings/AppSettings.h"

#include <string>
#include <windows.h>

namespace finderx::ui {

struct SettingsDialogValues {
    std::wstring fontSizeText;
    std::wstring iconSizeText;
};

bool promptForSettings(HWND owner, AppSettings& settings);
void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings);

} // namespace finderx::ui
```

Create `src/ui/SettingsDialog.cpp`:

```cpp
#include "ui/SettingsDialog.h"

#include <algorithm>
#include <cwchar>

namespace finderx::ui {
namespace {

float parseFloatOrKeep(const std::wstring& text, float currentValue) {
    wchar_t* end = nullptr;
    const float parsed = std::wcstof(text.c_str(), &end);
    if (end == text.c_str()) {
        return currentValue;
    }
    return parsed;
}

std::wstring numberText(float value) {
    wchar_t buffer[32]{};
    swprintf_s(buffer, L"%.0f", value);
    return buffer;
}

} // namespace

void applySettingsDialogValues(const SettingsDialogValues& values, AppSettings& settings) {
    settings.fontSize = parseFloatOrKeep(values.fontSizeText, settings.fontSize);
    settings.iconSize = parseFloatOrKeep(values.iconSizeText, settings.iconSize);
    clampSettings(settings);
}

} // namespace finderx::ui
```

In the same file, add `promptForSettings` using the existing no-resource modal pattern from `src/ui/RenameDialog.cpp`:

```cpp
bool promptForSettings(HWND owner, AppSettings& settings);
```

Required implementation details:

- Register a window class named `FinderXSettingsDialog`.
- Create the modal window with `CreateWindowExW`, `WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME`, and `WS_POPUP | WS_CAPTION | WS_SYSMENU`.
- Use `CreateWindowExW` child controls:
  - `STATIC` label `Font size:`;
  - `EDIT` for font size, initialized with `numberText(settings.fontSize)`;
  - `STATIC` label `Icon size:`;
  - `EDIT` for icon size, initialized with `numberText(settings.iconSize)`;
  - `OK` button;
  - `Cancel` button.
- Use `GetStockObject(DEFAULT_GUI_FONT)` with `WM_SETFONT` for every child control.
- Use a `SettingsDialogState` struct with:
  - `SettingsDialogValues values`;
  - `AppSettings initialSettings`;
  - `AppSettings resultSettings`;
  - `bool accepted = false`.
- On OK:
  - read both edit controls with `GetWindowTextW`;
  - call `applySettingsDialogValues`;
  - set `accepted = true`;
  - destroy the dialog.
- On Cancel, Escape, or close:
  - destroy the dialog without setting `accepted`.
- Use the same local message loop shape as `promptForRename`:
  - disable owner before showing the dialog;
  - use `IsDialogMessageW`;
  - re-enable owner and restore focus after the loop;
  - repost `WM_QUIT` if the loop receives it.
- Return `true` and assign `settings = state.resultSettings` only when `accepted` is true and the settings changed.
- Return `false` for cancel, close, unchanged values, window creation failure, or message-loop error.

- [ ] **Step 3: Register dialog tests**

Add to `CMakeLists.txt`:

```cmake
add_executable(finderx_settings_dialog_tests
    src/ui/SettingsDialogTests.cpp
    src/ui/SettingsDialog.cpp
)

target_include_directories(finderx_settings_dialog_tests PRIVATE src)
target_link_libraries(finderx_settings_dialog_tests PRIVATE finderx_settings)
finderx_enable_utf8(finderx_settings_dialog_tests)
finderx_enable_windows_unicode(finderx_settings_dialog_tests)
add_test(NAME finderx_settings_dialog_tests COMMAND finderx_settings_dialog_tests)
```

Add `src/ui/SettingsDialog.cpp` to `FINDERX_APP_SOURCES`.

- [ ] **Step 4: Wire Settings button**

In `MainWindow.h`, add:

```cpp
    void openSettingsDialog();
```

In `MainWindow.cpp`, include:

```cpp
#include "ui/SettingsDialog.h"
```

Implement:

```cpp
void MainWindow::openSettingsDialog() {
    AppSettings next = settings_;
    if (!ui::promptForSettings(hwnd_, next)) {
        return;
    }
    settings_ = std::move(next);
    clampSettings(settings_);
    saveSettingsOrStatus();
    applyVisualSettings();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

Change `ChromeHitKind::Settings` handler:

```cpp
        case ChromeHitKind::Settings:
            openSettingsDialog();
            return 0;
```

- [ ] **Step 5: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_settings_dialog_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_settings_dialog_tests --output-on-failure
```

Expected: build succeeds and dialog helper tests pass.

- [ ] **Step 6: Manual dialog smoke**

Run:

```powershell
Start-Process -FilePath "D:\finderX\build\Debug\FinderX.exe"
```

Verify:

1. Click `Settings`.
2. Change font size to `16` and icon size to `20`.
3. Confirm rows and text visibly update.
4. Close and restart.
5. Confirm sizes persist.

- [ ] **Step 7: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/ui/SettingsDialog.h src/ui/SettingsDialog.cpp src/ui/SettingsDialogTests.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add settings dialog"
```

---

### Task 7: Final Verification And Smoke

**Files:**
- No source changes expected.

- [ ] **Step 1: Run full Debug build and tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: Debug build succeeds and every CTest test passes.

- [ ] **Step 2: Run full Release build and tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

Expected: Release build succeeds and every CTest test passes.

- [ ] **Step 3: Run whitespace and status checks**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected:

- `git diff --check` prints no output.
- `git status --short` only shows known unrelated untracked files if they remain:
  - `docs/superpowers/plans/2026-06-29-finderx-cpp-mvp-plan.md`
  - `docs/superpowers/plans/2026-06-30-finderx-filesystem-loading-plan.md`
  - `docs/superpowers/specs/2026-06-29-finderx-cpp-design.md`
  - `template.png`

- [ ] **Step 4: Manual smoke**

Run:

```powershell
Start-Process -FilePath "D:\finderX\build\Release\FinderX.exe"
```

Verify:

1. Sidebar renders `Favorites` and `Locations`.
2. Home/Desktop/Documents/Downloads are under `Favorites`.
3. `This PC` and `Applications` are under `Locations`.
4. Right-click list background in a real directory and choose `Add to Favorites`.
5. Restart FinderX and confirm the favorite persists.
6. Right-click the added favorite and choose `Remove from Favorites`.
7. Click `Name`, `Date Modified`, `Size`, and `Kind` headers and confirm order changes.
8. Use the `Sort` toolbar menu to select modified descending.
9. Restart FinderX and confirm sort choice persists.
10. Open `Settings`, change font and icon sizes, confirm list visuals update.
11. Restart FinderX and confirm sizes persist.

- [ ] **Step 5: Commit smoke fixes only if needed**

If manual smoke reveals a source issue, fix it in the smallest relevant file set, rerun Debug and Release verification, then commit with a focused message.

---

## Final Verification Commands

Run these before reporting completion:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected:

- Debug build succeeds.
- Debug CTest passes.
- Release build succeeds.
- Release CTest passes.
- `git diff --check` has no output.
- Remaining untracked files, if present, match the known unrelated files listed in Task 7.
