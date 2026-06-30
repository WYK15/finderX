#include "ui/RenameDialog.h"

namespace finderx::ui {

bool isValidRenameName(const std::wstring& name) {
    if (name.empty()) {
        return false;
    }

    return name.find_first_of(LR"(\/:*?"<>|)") == std::wstring::npos;
}

bool promptForRename(HWND, const std::wstring&, std::wstring&) {
    return false;
}

} // namespace finderx::ui
