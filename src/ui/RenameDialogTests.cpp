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

    const std::wstring invalidChars = LR"(\/:*?"<>|)";
    for (wchar_t invalidChar : invalidChars) {
        std::wstring name = L"file";
        name.push_back(invalidChar);
        name += L".txt";
        require(!isValidRenameName(name), "invalid character should reject filename");
    }

    std::cout << "RenameDialogTests passed\n";
    return 0;
}
