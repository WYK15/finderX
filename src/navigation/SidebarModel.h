#pragma once

#include "settings/AppSettings.h"

#include <cstddef>
#include <string>
#include <vector>

namespace finderx {

enum class SidebarItemRole {
    Favorite,
    Location
};

struct SidebarItem {
    std::wstring label;
    std::wstring path;
    SidebarItemRole role = SidebarItemRole::Favorite;
    bool available = false;
    bool selected = false;
};

struct ChromeState {
    std::wstring title;
    std::wstring pathText;
    std::wstring statusText;
    bool addressEditing = false;
    std::wstring addressText;
    std::size_t addressCaretIndex = 0;
    std::size_t addressSelectionStart = 0;
    std::size_t addressSelectionEnd = 0;
    bool addressCaretVisible = false;
    bool canGoBack = false;
    bool canGoForward = false;
    std::vector<SidebarItem> sidebarItems;
    std::wstring searchText;
    bool searchFocused = false;
    bool searchCaretVisible = false;
    std::vector<std::wstring> tabTitles;
    std::size_t activeTabIndex = 0;
    SortColumn sortColumn = SortColumn::Name;
    SortDirection sortDirection = SortDirection::Ascending;
    ThemeMode themeMode = ThemeMode::Dark;
    float modifiedColumnWidth = kDefaultModifiedColumnWidth;
    float sizeColumnWidth = kDefaultSizeColumnWidth;
    float kindColumnWidth = kDefaultKindColumnWidth;
};

class SidebarModel {
public:
    void refresh(const std::wstring& homePath, const std::wstring& currentPath, const std::vector<FavoriteLocation>& favorites);
    const std::vector<SidebarItem>& items() const;
    void setAvailabilityForTests(const std::wstring& label, bool available);

private:
    std::vector<SidebarItem> items_;
};

std::wstring joinPathForNavigation(const std::wstring& base, const std::wstring& child);
std::wstring thisPcPath();
std::wstring userProgramsDirectory();

} // namespace finderx
