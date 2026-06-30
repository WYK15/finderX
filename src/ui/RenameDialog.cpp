#include "ui/RenameDialog.h"

namespace finderx::ui {

namespace {

std::wstring upperAscii(std::wstring value) {
    for (wchar_t& ch : value) {
        if (ch >= L'a' && ch <= L'z') {
            ch = static_cast<wchar_t>(ch - L'a' + L'A');
        }
    }
    return value;
}

bool isReservedDosDeviceName(const std::wstring& name) {
    const std::size_t extensionOffset = name.find(L'.');
    const std::wstring baseName = upperAscii(name.substr(0, extensionOffset));

    if (baseName == L"CON" || baseName == L"PRN" || baseName == L"AUX" || baseName == L"NUL") {
        return true;
    }

    if (baseName.size() == 4 && (baseName.starts_with(L"COM") || baseName.starts_with(L"LPT"))) {
        return baseName[3] >= L'1' && baseName[3] <= L'9';
    }

    return false;
}

} // namespace

bool isValidRenameName(const std::wstring& name) {
    if (name.empty()) {
        return false;
    }

    if (name == L"." || name == L"..") {
        return false;
    }

    if (name.back() == L' ' || name.back() == L'.') {
        return false;
    }

    if (name.find_first_of(LR"(\/:*?"<>|)") != std::wstring::npos) {
        return false;
    }

    for (wchar_t ch : name) {
        if (ch < 0x20) {
            return false;
        }
    }

    return !isReservedDosDeviceName(name);
}

bool promptForRename(HWND, const std::wstring&, std::wstring&) {
    return false;
}

} // namespace finderx::ui
