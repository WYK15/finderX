#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace finderx::ui {

struct TabInfo {
    std::wstring path;
    std::wstring title;
    std::wstring searchText;
    std::wstring statusText;
};

class TabManager {
public:
    void initialize(std::wstring path) {
        tabs_.clear();
        activeIndex_ = 0;
        tabs_.push_back(makeTab(std::move(path)));
    }

    std::size_t createTab(std::wstring path) {
        tabs_.push_back(makeTab(std::move(path)));
        activeIndex_ = tabs_.size() - 1;
        return activeIndex_;
    }

    void activate(std::size_t index) {
        if (index < tabs_.size()) {
            activeIndex_ = index;
        }
    }

    std::size_t count() const {
        return tabs_.size();
    }

    std::size_t activeIndex() const {
        return activeIndex_;
    }

    TabInfo& active() {
        return tabs_[activeIndex_];
    }

    const TabInfo& active() const {
        return tabs_[activeIndex_];
    }

    const std::vector<TabInfo>& tabs() const {
        return tabs_;
    }

private:
    static std::wstring titleFromPath(const std::wstring& path) {
        const std::size_t end = path.find_last_not_of(L"\\/");
        if (end == std::wstring::npos) {
            return path;
        }
        const std::size_t slash = path.find_last_of(L"\\/", end);
        if (slash == std::wstring::npos) {
            if (end + 1 < path.size()) {
                return path;
            }
            return path.substr(0, end + 1);
        }
        std::wstring title = path.substr(slash + 1, end - slash);
        return title.empty() ? path : title;
    }

    static TabInfo makeTab(std::wstring path) {
        TabInfo tab;
        tab.path = std::move(path);
        tab.title = titleFromPath(tab.path);
        return tab;
    }

    std::vector<TabInfo> tabs_;
    std::size_t activeIndex_ = 0;
};

} // namespace finderx::ui
