#pragma once

#include <string>
#include <vector>

namespace finderx {

class NavigationHistory {
public:
    void setInitialPath(std::wstring path);
    void navigateTo(std::wstring path);

    bool canGoBack() const;
    bool canGoForward() const;

    std::wstring backTarget() const;
    std::wstring forwardTarget() const;

    std::wstring goBack();
    std::wstring goForward();

    const std::wstring& currentPath() const;

private:
    std::wstring currentPath_;
    std::vector<std::wstring> backStack_;
    std::vector<std::wstring> forwardStack_;
};

} // namespace finderx
