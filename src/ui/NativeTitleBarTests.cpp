#include "ui/NativeTitleBar.h"

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
    using finderx::ThemeMode;
    using finderx::ui::shouldUseDarkTitleBar;

    require(shouldUseDarkTitleBar(ThemeMode::Dark), "dark theme should request dark native title bar");
    require(!shouldUseDarkTitleBar(ThemeMode::Light), "light theme should request light native title bar");

    std::cout << "NativeTitleBarTests passed\n";
    return 0;
}
