#include "settings/AppSettings.h"

#include <Windows.h>

#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string_view>

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

std::wstring favoriteLabelFromPath(const std::wstring& path) {
    const std::size_t end = path.find_last_not_of(L"\\/");
    if (end == std::wstring::npos) {
        return path;
    }
    const std::size_t slash = path.find_last_of(L"\\/", end);
    if (slash == std::wstring::npos) {
        return path.substr(0, end + 1);
    }
    std::wstring label = path.substr(slash + 1, end - slash);
    return label.empty() ? path : label;
}

std::string wideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8,
                                             0,
                                             value.c_str(),
                                             static_cast<int>(value.size()),
                                             nullptr,
                                             0,
                                             nullptr,
                                             nullptr);
    if (required <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8,
                        0,
                        value.c_str(),
                        static_cast<int>(value.size()),
                        result.data(),
                        required,
                        nullptr,
                        nullptr);
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
    const std::string utf8 = wideToUtf8(value);
    std::string escaped;
    escaped.reserve(utf8.size());

    for (const char ch : utf8) {
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

bool hasObjectBraces(std::string_view text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return first != std::string_view::npos && text[first] == '{' && text[last] == '}';
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

    std::size_t start = colon + 1;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    if (start == text.size() || (text[start] != '-' && text[start] != '+' && !std::isdigit(static_cast<unsigned char>(text[start])))) {
        return false;
    }

    char* end = nullptr;
    const float parsed = std::strtof(text.c_str() + start, &end);
    if (end == text.c_str() + start) {
        return false;
    }

    const char* cursor = end;
    while (*cursor != '\0' && std::isspace(static_cast<unsigned char>(*cursor))) {
        ++cursor;
    }
    if (*cursor != ',' && *cursor != '}' && *cursor != '\n' && *cursor != '\r') {
        return false;
    }

    value = parsed;
    return true;
}

bool extractBool(const std::string& text, const std::string& key, bool& value) {
    const std::string marker = "\"" + key + "\"";
    const std::size_t keyPos = text.find(marker);
    if (keyPos == std::string::npos) {
        return false;
    }

    const std::size_t colon = text.find(':', keyPos + marker.size());
    if (colon == std::string::npos) {
        return false;
    }

    std::size_t start = colon + 1;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    if (text.compare(start, 4, "true") == 0) {
        value = true;
        return true;
    }
    if (text.compare(start, 5, "false") == 0) {
        value = false;
        return true;
    }
    return false;
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

ThemeMode parseThemeMode(const std::wstring& value, ThemeMode fallback) {
    if (value == L"light") {
        return ThemeMode::Light;
    }
    if (value == L"dark") {
        return ThemeMode::Dark;
    }
    return fallback;
}

ToolbarCommand parseToolbarCommand(const std::wstring& value, ToolbarCommand fallback) {
    if (value == L"newFolder") {
        return ToolbarCommand::NewFolder;
    }
    if (value == L"newFile") {
        return ToolbarCommand::NewFile;
    }
    if (value == L"sort") {
        return ToolbarCommand::Sort;
    }
    if (value == L"settings") {
        return ToolbarCommand::Settings;
    }
    if (value == L"search") {
        return ToolbarCommand::Search;
    }
    return fallback;
}

struct FavoritesParseResult {
    bool present = false;
    std::vector<FavoriteLocation> favorites;
};

struct StringArrayParseResult {
    bool present = false;
    std::vector<std::wstring> values;
};

std::size_t findStructuralChar(const std::string& text, char target, std::size_t start) {
    bool inString = false;
    bool escaping = false;
    for (std::size_t index = start; index < text.size(); ++index) {
        const char ch = text[index];
        if (escaping) {
            escaping = false;
            continue;
        }
        if (inString && ch == '\\') {
            escaping = true;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (!inString && ch == target) {
            return index;
        }
    }
    return std::string::npos;
}

FavoritesParseResult parseFavorites(const std::string& text) {
    FavoritesParseResult result;
    const std::size_t favoritesKey = text.find("\"favorites\"");
    if (favoritesKey == std::string::npos) {
        return result;
    }
    result.present = true;

    const std::size_t arrayStart = findStructuralChar(text, '[', favoritesKey);
    if (arrayStart == std::string::npos) {
        return result;
    }

    const std::size_t arrayEnd = findStructuralChar(text, ']', arrayStart + 1);
    if (arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        return result;
    }

    std::size_t cursor = arrayStart + 1;
    while (cursor < arrayEnd) {
        const std::size_t objectStart = findStructuralChar(text, '{', cursor);
        if (objectStart == std::string::npos || objectStart >= arrayEnd) {
            break;
        }

        const std::size_t objectEnd = findStructuralChar(text, '}', objectStart + 1);
        if (objectEnd == std::string::npos || objectEnd > arrayEnd) {
            break;
        }

        const std::string object = text.substr(objectStart, objectEnd - objectStart + 1);
        FavoriteLocation favorite;
        if (extractString(object, "label", favorite.label) && extractString(object, "path", favorite.path) && !favorite.path.empty()) {
            result.favorites.push_back(std::move(favorite));
        }
        cursor = objectEnd + 1;
    }

    return result;
}

StringArrayParseResult parseStringArray(const std::string& text, const std::string& key) {
    StringArrayParseResult result;
    const std::size_t keyPos = text.find("\"" + key + "\"");
    if (keyPos == std::string::npos) {
        return result;
    }
    result.present = true;

    const std::size_t arrayStart = findStructuralChar(text, '[', keyPos);
    if (arrayStart == std::string::npos) {
        return result;
    }
    const std::size_t arrayEnd = findStructuralChar(text, ']', arrayStart + 1);
    if (arrayEnd == std::string::npos || arrayEnd <= arrayStart) {
        return result;
    }

    std::size_t cursor = arrayStart + 1;
    while (cursor < arrayEnd) {
        const std::size_t quote = text.find('"', cursor);
        if (quote == std::string::npos || quote >= arrayEnd) {
            break;
        }

        std::string raw;
        bool escaping = false;
        std::size_t index = quote + 1;
        for (; index < arrayEnd; ++index) {
            const char ch = text[index];
            if (escaping) {
                raw.push_back(ch);
                escaping = false;
                continue;
            }
            if (ch == '\\') {
                escaping = true;
                continue;
            }
            if (ch == '"') {
                result.values.push_back(utf8ToWide(raw));
                break;
            }
            raw.push_back(ch);
        }
        cursor = index + 1;
    }

    return result;
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
    if (settings.fontFamily.empty()) {
        settings.fontFamily = kDefaultFontFamily;
    }
    settings.iconSize = (std::clamp)(settings.iconSize, kMinIconSize, kMaxIconSize);
    settings.modifiedColumnWidth = (std::clamp)(settings.modifiedColumnWidth, kMinModifiedColumnWidth, kMaxModifiedColumnWidth);
    settings.sizeColumnWidth = (std::clamp)(settings.sizeColumnWidth, kMinSizeColumnWidth, kMaxSizeColumnWidth);
    settings.kindColumnWidth = (std::clamp)(settings.kindColumnWidth, kMinKindColumnWidth, kMaxKindColumnWidth);
    settings.windowWidth = (std::clamp)(settings.windowWidth, kMinWindowWidth, kMaxWindowWidth);
    settings.windowHeight = (std::clamp)(settings.windowHeight, kMinWindowHeight, kMaxWindowHeight);
    settings.toolbarCommands = normalizeToolbarCommands(std::move(settings.toolbarCommands));
}

bool addFavorite(AppSettings& settings, std::wstring label, std::wstring path) {
    if (path.empty() || containsFavorite(settings, path)) {
        return false;
    }

    settings.favorites.push_back({std::move(label), std::move(path)});
    return true;
}

bool removeFavorite(AppSettings& settings, const std::wstring& path) {
    const auto originalEnd = settings.favorites.end();
    const auto newEnd = std::remove_if(settings.favorites.begin(), settings.favorites.end(), [&](const FavoriteLocation& favorite) {
        return pathsEqual(favorite.path, path);
    });
    const bool removed = newEnd != originalEnd;
    settings.favorites.erase(newEnd, originalEnd);
    return removed;
}

bool containsFavorite(const AppSettings& settings, const std::wstring& path) {
    return std::any_of(settings.favorites.begin(), settings.favorites.end(), [&](const FavoriteLocation& favorite) {
        return pathsEqual(favorite.path, path);
    });
}

bool renameFavorite(AppSettings& settings, std::size_t index, std::wstring label) {
    if (index >= settings.favorites.size()) {
        return false;
    }
    if (label.empty()) {
        label = favoriteLabelFromPath(settings.favorites[index].path);
    }
    settings.favorites[index].label = std::move(label);
    return true;
}

bool moveFavorite(AppSettings& settings, std::size_t index, int direction) {
    if (index >= settings.favorites.size() || (direction != -1 && direction != 1)) {
        return false;
    }
    if (direction < 0 && index == 0) {
        return false;
    }
    const std::size_t next = direction < 0 ? index - 1 : index + 1;
    if (next >= settings.favorites.size()) {
        return false;
    }
    std::swap(settings.favorites[index], settings.favorites[next]);
    return true;
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

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    const std::string text = buffer.str();
    if (!hasObjectBraces(text)) {
        return result;
    }

    AppSettings loaded = result.settings;
    float number = 0.0f;
    std::wstring stringValue;
    if (extractNumber(text, "fontSize", number)) {
        loaded.fontSize = number;
    }
    if (extractString(text, "fontFamily", stringValue) && !stringValue.empty()) {
        loaded.fontFamily = std::move(stringValue);
    }
    if (extractNumber(text, "iconSize", number)) {
        loaded.iconSize = number;
    }
    if (extractNumber(text, "modifiedColumnWidth", number)) {
        loaded.modifiedColumnWidth = number;
    }
    if (extractNumber(text, "sizeColumnWidth", number)) {
        loaded.sizeColumnWidth = number;
    }
    if (extractNumber(text, "kindColumnWidth", number)) {
        loaded.kindColumnWidth = number;
    }
    if (extractNumber(text, "windowWidth", number)) {
        loaded.windowWidth = static_cast<int>(number);
    }
    if (extractNumber(text, "windowHeight", number)) {
        loaded.windowHeight = static_cast<int>(number);
    }

    if (extractString(text, "sortColumn", stringValue)) {
        loaded.sortColumn = parseSortColumn(stringValue);
    }
    if (extractString(text, "sortDirection", stringValue)) {
        loaded.sortDirection = parseSortDirection(stringValue);
    }
    if (extractString(text, "themeMode", stringValue)) {
        loaded.themeMode = parseThemeMode(stringValue, loaded.themeMode);
    }
    bool boolValue = false;
    if (extractBool(text, "showHiddenAndSystemItems", boolValue)) {
        loaded.showHiddenAndSystemItems = boolValue;
    }
    if (extractBool(text, "rememberWindowSize", boolValue)) {
        loaded.rememberWindowSize = boolValue;
    }

    FavoritesParseResult favorites = parseFavorites(text);
    if (favorites.present) {
        loaded.favorites = std::move(favorites.favorites);
    }

    StringArrayParseResult toolbarItems = parseStringArray(text, "toolbarCommands");
    if (toolbarItems.present) {
        loaded.toolbarCommands.clear();
        for (const std::wstring& value : toolbarItems.values) {
            loaded.toolbarCommands.push_back(parseToolbarCommand(value, ToolbarCommand::Search));
        }
    }

    clampSettings(loaded);
    result.settings = std::move(loaded);
    result.loadedFromDisk = true;
    return result;
}

bool saveSettings(const AppSettings& settings, const std::filesystem::path& path) {
    std::error_code error;
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            return false;
        }
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return false;
    }

    AppSettings clamped = settings;
    clampSettings(clamped);

    stream << "{\n";
    stream << "  \"fontSize\": " << clamped.fontSize << ",\n";
    stream << "  \"fontFamily\": \"" << escapeJsonString(clamped.fontFamily) << "\",\n";
    stream << "  \"iconSize\": " << clamped.iconSize << ",\n";
    stream << "  \"modifiedColumnWidth\": " << clamped.modifiedColumnWidth << ",\n";
    stream << "  \"sizeColumnWidth\": " << clamped.sizeColumnWidth << ",\n";
    stream << "  \"kindColumnWidth\": " << clamped.kindColumnWidth << ",\n";
    stream << "  \"windowWidth\": " << clamped.windowWidth << ",\n";
    stream << "  \"windowHeight\": " << clamped.windowHeight << ",\n";
    stream << "  \"rememberWindowSize\": " << (clamped.rememberWindowSize ? "true" : "false") << ",\n";
    stream << "  \"themeMode\": \"" << escapeJsonString(themeModeName(clamped.themeMode)) << "\",\n";
    stream << "  \"showHiddenAndSystemItems\": " << (clamped.showHiddenAndSystemItems ? "true" : "false") << ",\n";
    stream << "  \"sortColumn\": \"" << escapeJsonString(sortColumnName(clamped.sortColumn)) << "\",\n";
    stream << "  \"sortDirection\": \"" << escapeJsonString(sortDirectionName(clamped.sortDirection)) << "\",\n";
    stream << "  \"toolbarCommands\": [";
    for (std::size_t index = 0; index < clamped.toolbarCommands.size(); ++index) {
        if (index > 0) {
            stream << ", ";
        }
        stream << "\"" << escapeJsonString(toolbarCommandName(clamped.toolbarCommands[index])) << "\"";
    }
    stream << "],\n";
    stream << "  \"favorites\": [\n";
    for (std::size_t index = 0; index < clamped.favorites.size(); ++index) {
        const FavoriteLocation& favorite = clamped.favorites[index];
        stream << "    {\"label\": \"" << escapeJsonString(favorite.label) << "\", \"path\": \"" << escapeJsonString(favorite.path) << "\"}";
        if (index + 1 < clamped.favorites.size()) {
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
    switch (direction) {
    case SortDirection::Descending:
        return L"descending";
    case SortDirection::Ascending:
    default:
        return L"ascending";
    }
}

std::wstring themeModeName(ThemeMode mode) {
    switch (mode) {
    case ThemeMode::Light:
        return L"light";
    case ThemeMode::Dark:
    default:
        return L"dark";
    }
}

std::wstring toolbarCommandName(ToolbarCommand command) {
    switch (command) {
    case ToolbarCommand::NewFolder:
        return L"newFolder";
    case ToolbarCommand::NewFile:
        return L"newFile";
    case ToolbarCommand::Sort:
        return L"sort";
    case ToolbarCommand::Settings:
        return L"settings";
    case ToolbarCommand::Search:
    default:
        return L"search";
    }
}

std::vector<ToolbarCommand> defaultToolbarCommands() {
    return {
        ToolbarCommand::NewFolder,
        ToolbarCommand::NewFile,
        ToolbarCommand::Sort,
        ToolbarCommand::Settings,
        ToolbarCommand::Search,
    };
}

std::vector<ToolbarCommand> normalizeToolbarCommands(std::vector<ToolbarCommand> commands) {
    std::vector<ToolbarCommand> normalized;
    normalized.reserve(commands.size());
    for (const ToolbarCommand command : commands) {
        if (std::find(normalized.begin(), normalized.end(), command) == normalized.end()) {
            normalized.push_back(command);
        }
    }
    if (normalized.empty()) {
        return defaultToolbarCommands();
    }
    return normalized;
}

} // namespace finderx
