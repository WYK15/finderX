#pragma once

#include <string>
#include <vector>

namespace finderx {

struct NavigationHistoryEntry {
    std::wstring path;
    std::vector<std::wstring> expandedPaths;
};

class NavigationHistory {
public:
    void setInitialPath(std::wstring path);
    void setInitialPath(std::wstring path, std::vector<std::wstring> expandedPaths);
    void navigateTo(std::wstring path);
    void navigateTo(std::wstring path, std::vector<std::wstring> currentExpandedPaths);
    void updateCurrentExpandedPaths(std::vector<std::wstring> expandedPaths);

    bool canGoBack() const;
    bool canGoForward() const;

    std::wstring backTarget() const;
    std::wstring forwardTarget() const;
    const NavigationHistoryEntry& currentEntry() const;

    std::wstring goBack();
    std::wstring goForward();
    NavigationHistoryEntry goBack(std::vector<std::wstring> currentExpandedPaths);
    NavigationHistoryEntry goForward(std::vector<std::wstring> currentExpandedPaths);

    const std::wstring& currentPath() const;

private:
    NavigationHistoryEntry currentEntry_;
    std::vector<NavigationHistoryEntry> backStack_;
    std::vector<NavigationHistoryEntry> forwardStack_;
};

} // namespace finderx
