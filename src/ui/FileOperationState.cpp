#include "ui/FileOperationState.h"

#include <utility>

namespace finderx::ui {

void FileOperationState::setCopy(std::wstring path) {
    if (path.empty()) {
        clear();
        return;
    }

    kind_ = PendingFileOperationKind::Copy;
    sourcePath_ = std::move(path);
}

void FileOperationState::setCut(std::wstring path) {
    if (path.empty()) {
        clear();
        return;
    }

    kind_ = PendingFileOperationKind::Cut;
    sourcePath_ = std::move(path);
}

void FileOperationState::clear() {
    kind_ = PendingFileOperationKind::None;
    sourcePath_.clear();
}

void FileOperationState::markPasteSucceeded() {
    if (kind_ == PendingFileOperationKind::Cut) {
        clear();
    }
}

bool FileOperationState::hasPendingOperation() const {
    return kind_ != PendingFileOperationKind::None && !sourcePath_.empty();
}

PendingFileOperationKind FileOperationState::kind() const {
    return kind_;
}

const std::wstring& FileOperationState::sourcePath() const {
    return sourcePath_;
}

} // namespace finderx::ui
