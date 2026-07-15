#include "navigation/NavigationHistory.h"

#include <utility>

namespace finderx {

void NavigationHistory::setInitialPath(std::wstring path) {
    setInitialPath(std::move(path), {});
}

void NavigationHistory::setInitialPath(std::wstring path, std::vector<std::wstring> expandedPaths) {
    currentEntry_.path = std::move(path);
    currentEntry_.expandedPaths = std::move(expandedPaths);
    backStack_.clear();
    forwardStack_.clear();
}

void NavigationHistory::navigateTo(std::wstring path) {
    navigateTo(std::move(path), currentEntry_.expandedPaths);
}

void NavigationHistory::navigateTo(std::wstring path, std::vector<std::wstring> currentExpandedPaths) {
    if (path.empty() || path == currentEntry_.path) {
        return;
    }

    currentEntry_.expandedPaths = std::move(currentExpandedPaths);
    if (!currentEntry_.path.empty()) {
        backStack_.push_back(std::move(currentEntry_));
    }

    currentEntry_ = {};
    currentEntry_.path = std::move(path);
    forwardStack_.clear();
}

void NavigationHistory::updateCurrentExpandedPaths(std::vector<std::wstring> expandedPaths) {
    currentEntry_.expandedPaths = std::move(expandedPaths);
}

bool NavigationHistory::canGoBack() const {
    return !backStack_.empty();
}

bool NavigationHistory::canGoForward() const {
    return !forwardStack_.empty();
}

std::wstring NavigationHistory::backTarget() const {
    return canGoBack() ? backStack_.back().path : currentEntry_.path;
}

std::wstring NavigationHistory::forwardTarget() const {
    return canGoForward() ? forwardStack_.back().path : currentEntry_.path;
}

const NavigationHistoryEntry& NavigationHistory::currentEntry() const {
    return currentEntry_;
}

std::wstring NavigationHistory::goBack() {
    return goBack(currentEntry_.expandedPaths).path;
}

std::wstring NavigationHistory::goForward() {
    return goForward(currentEntry_.expandedPaths).path;
}

NavigationHistoryEntry NavigationHistory::goBack(std::vector<std::wstring> currentExpandedPaths) {
    if (!canGoBack()) {
        return currentEntry_;
    }

    currentEntry_.expandedPaths = std::move(currentExpandedPaths);
    forwardStack_.push_back(std::move(currentEntry_));
    currentEntry_ = std::move(backStack_.back());
    backStack_.pop_back();
    return currentEntry_;
}

NavigationHistoryEntry NavigationHistory::goForward(std::vector<std::wstring> currentExpandedPaths) {
    if (!canGoForward()) {
        return currentEntry_;
    }

    currentEntry_.expandedPaths = std::move(currentExpandedPaths);
    backStack_.push_back(std::move(currentEntry_));
    currentEntry_ = std::move(forwardStack_.back());
    forwardStack_.pop_back();
    return currentEntry_;
}

const std::wstring& NavigationHistory::currentPath() const {
    return currentEntry_.path;
}

} // namespace finderx
