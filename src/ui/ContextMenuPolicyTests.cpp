#include "ui/ContextMenuPolicy.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    using finderx::ui::shouldShowCurrentDirectoryActions;

    require(shouldShowCurrentDirectoryActions(false, true),
            "directory background context menu should show current directory actions");
    require(shouldShowCurrentDirectoryActions(true, true),
            "file context menu in a directory should still show current directory actions");
    require(!shouldShowCurrentDirectoryActions(false, false),
            "virtual locations should not show current directory actions");
    require(!shouldShowCurrentDirectoryActions(true, false),
            "virtual location item context menu should not show current directory actions");

    std::cout << "ContextMenuPolicyTests passed\n";
    return 0;
}
