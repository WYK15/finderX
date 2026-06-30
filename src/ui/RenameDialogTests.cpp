#include "ui/RenameDialog.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using finderx::ui::isValidRenameName;

    require(isValidRenameName(L"Report.txt"), "Report.txt should be valid");
    require(isValidRenameName(L"项目.txt"), "Unicode filename should be valid");
    require(!isValidRenameName(L""), "empty name should be invalid");
    require(!isValidRenameName(L"."), "single dot should be invalid");
    require(!isValidRenameName(L".."), "double dot should be invalid");
    require(!isValidRenameName(L"Report.txt "), "trailing space should be invalid");
    require(!isValidRenameName(L"Report.txt."), "trailing dot should be invalid");
    require(!isValidRenameName(L"line\nbreak.txt"), "ASCII control character should be invalid");

    const std::wstring invalidChars = LR"(\/:*?"<>|)";
    for (wchar_t invalidChar : invalidChars) {
        std::wstring name = L"file";
        name.push_back(invalidChar);
        name += L".txt";
        require(!isValidRenameName(name), "invalid character should reject filename");
    }

    const std::wstring reservedNames[] = {
        L"CON",
        L"con",
        L"NUL.txt",
        L"com1.log",
        L"LPT9",
        L"PRN",
        L"AUX",
        L"COM9",
        L"LPT1.txt",
    };
    for (const std::wstring& reservedName : reservedNames) {
        require(!isValidRenameName(reservedName), "reserved DOS device name should be invalid");
    }

    std::cout << "RenameDialogTests passed\n";
    return 0;
}
