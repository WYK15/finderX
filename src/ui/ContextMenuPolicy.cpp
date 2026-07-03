#include "ui/ContextMenuPolicy.h"

namespace finderx::ui {

bool shouldShowCurrentDirectoryActions(bool, bool isDirectoryLocation) {
    return isDirectoryLocation;
}

} // namespace finderx::ui
