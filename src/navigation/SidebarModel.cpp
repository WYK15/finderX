#include "navigation/SidebarModel.h"

#include <Windows.h>

#include <cwchar>

namespace finderx {
namespace {

std::wstring environmentVariable(const std::wstring& name) {
    const DWORD requiredLength = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
    if (requiredLength == 0) {
        return L"";
    }

    std::wstring value(requiredLength, L'\0');
    const DWORD writtenLength = GetEnvironmentVariableW(name.c_str(), value.data(), requiredLength);
    if (writtenLength == 0 || writtenLength >= requiredLength) {
        return L"";
    }

    value.resize(writtenLength);
    return value;
}

bool pathsEqual(const std::wstring& left, const std::wstring& right) {
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

SidebarItem makeFavoriteItem(const FavoriteLocation& favorite, const std::wstring& currentPath) {
    SidebarItem item;
    item.label = favorite.label;
    item.path = favorite.path;
    item.role = SidebarItemRole::Favorite;
    item.available = !item.path.empty();
    item.selected = item.available && pathsEqual(item.path, currentPath);
    return item;
}

SidebarItem makeThisPcItem(const std::wstring& currentPath) {
    SidebarItem item;
    item.label = L"This PC";
    item.path = thisPcPath();
    item.role = SidebarItemRole::Location;
    item.available = true;
    item.selected = pathsEqual(item.path, currentPath);
    return item;
}

} // namespace

std::wstring joinPathForNavigation(const std::wstring& base, const std::wstring& child) {
    if (base.empty()) {
        return child;
    }

    const wchar_t lastCharacter = base.back();
    if (lastCharacter == L'\\' || lastCharacter == L'/') {
        return base + child;
    }

    return base + L"\\" + child;
}

std::wstring thisPcPath() {
    return L"finderx://this-pc";
}

std::wstring userProgramsDirectory() {
    const std::wstring appData = environmentVariable(L"APPDATA");
    if (appData.empty()) {
        return L"";
    }

    return joinPathForNavigation(appData, L"Microsoft\\Windows\\Start Menu\\Programs");
}

void SidebarModel::refresh(const std::wstring& homePath, const std::wstring& currentPath, const std::vector<FavoriteLocation>& favorites) {
    (void)homePath;
    items_.clear();
    items_.reserve(favorites.size() + 1);
    for (const FavoriteLocation& favorite : favorites) {
        items_.push_back(makeFavoriteItem(favorite, currentPath));
    }
    items_.push_back(makeThisPcItem(currentPath));
}

const std::vector<SidebarItem>& SidebarModel::items() const {
    return items_;
}

void SidebarModel::setAvailabilityForTests(const std::wstring& label, bool available) {
    for (auto& item : items_) {
        if (item.label == label) {
            item.available = available;
        }
    }
}

} // namespace finderx
