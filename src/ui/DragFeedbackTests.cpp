#include "ui/DragFeedback.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    require(ui::dragFeedbackText(false, 1, L"Documents") == L"Cannot move here",
            "invalid target should show cannot move text");
    require(ui::dragFeedbackText(true, 1, L"Documents") == L"Move to Documents",
            "single item valid target should name destination");
    require(ui::dragFeedbackText(true, 3, L"Documents") == L"Move 3 items",
            "multi item valid target should show item count");

    std::cout << "DragFeedbackTests passed\n";
    return 0;
}
