#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace finderx {

struct SidebarItem {
    std::wstring label;
    std::wstring path;
    bool available = false;
    bool selected = false;
};

struct ChromeState {
    std::wstring title;
    std::wstring pathText;
    std::wstring statusText;
    bool canGoBack = false;
    bool canGoForward = false;
    std::vector<SidebarItem> sidebarItems;
    std::wstring searchText;
    bool searchFocused = false;
    std::vector<std::wstring> tabTitles;
    std::size_t activeTabIndex = 0;
};

class SidebarModel {
public:
    void refresh(const std::wstring& homePath, const std::wstring& currentPath);
    const std::vector<SidebarItem>& items() const;
    void setAvailabilityForTests(const std::wstring& label, bool available);

private:
    std::vector<SidebarItem> items_;
};

std::wstring joinPathForNavigation(const std::wstring& base, const std::wstring& child);
std::wstring thisPcPath();
std::wstring userProgramsDirectory();

} // namespace finderx
