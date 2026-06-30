#include "navigation/NavigationHistory.h"

#include <utility>

namespace finderx {

void NavigationHistory::setInitialPath(std::wstring path) {
    currentPath_ = std::move(path);
    backStack_.clear();
    forwardStack_.clear();
}

void NavigationHistory::navigateTo(std::wstring path) {
    if (path.empty() || path == currentPath_) {
        return;
    }

    if (!currentPath_.empty()) {
        backStack_.push_back(currentPath_);
    }

    currentPath_ = std::move(path);
    forwardStack_.clear();
}

bool NavigationHistory::canGoBack() const {
    return !backStack_.empty();
}

bool NavigationHistory::canGoForward() const {
    return !forwardStack_.empty();
}

std::wstring NavigationHistory::backTarget() const {
    return canGoBack() ? backStack_.back() : currentPath_;
}

std::wstring NavigationHistory::forwardTarget() const {
    return canGoForward() ? forwardStack_.back() : currentPath_;
}

std::wstring NavigationHistory::goBack() {
    if (!canGoBack()) {
        return currentPath_;
    }

    forwardStack_.push_back(currentPath_);
    currentPath_ = std::move(backStack_.back());
    backStack_.pop_back();
    return currentPath_;
}

std::wstring NavigationHistory::goForward() {
    if (!canGoForward()) {
        return currentPath_;
    }

    backStack_.push_back(currentPath_);
    currentPath_ = std::move(forwardStack_.back());
    forwardStack_.pop_back();
    return currentPath_;
}

const std::wstring& NavigationHistory::currentPath() const {
    return currentPath_;
}

} // namespace finderx
