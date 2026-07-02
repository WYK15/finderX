#include "ui/DragFeedback.h"

namespace finderx::ui {

std::wstring dragFeedbackText(bool canMove, std::size_t itemCount, const std::wstring& destinationName) {
    if (!canMove) {
        return L"Cannot move here";
    }

    if (itemCount > 1) {
        return L"Move " + std::to_wstring(itemCount) + L" items";
    }

    if (!destinationName.empty()) {
        return L"Move to " + destinationName;
    }

    return L"Move item";
}

}
