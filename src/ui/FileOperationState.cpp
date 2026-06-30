#include "ui/FileOperationState.h"

#include <utility>

namespace finderx::ui {

namespace {

std::vector<std::wstring> cleanedPaths(std::vector<std::wstring> paths) {
    std::vector<std::wstring> cleaned;
    cleaned.reserve(paths.size());

    for (auto& path : paths) {
        if (!path.empty()) {
            cleaned.push_back(std::move(path));
        }
    }

    return cleaned;
}

} // namespace

void FileOperationState::setCopy(std::wstring path) {
    std::vector<std::wstring> paths;
    paths.push_back(std::move(path));
    setCopy(std::move(paths));
}

void FileOperationState::setCopy(std::vector<std::wstring> paths) {
    auto cleaned = cleanedPaths(std::move(paths));
    if (cleaned.empty()) {
        clear();
        return;
    }

    kind_ = PendingFileOperationKind::Copy;
    sourcePaths_ = std::move(cleaned);
}

void FileOperationState::setCut(std::wstring path) {
    std::vector<std::wstring> paths;
    paths.push_back(std::move(path));
    setCut(std::move(paths));
}

void FileOperationState::setCut(std::vector<std::wstring> paths) {
    auto cleaned = cleanedPaths(std::move(paths));
    if (cleaned.empty()) {
        clear();
        return;
    }

    kind_ = PendingFileOperationKind::Cut;
    sourcePaths_ = std::move(cleaned);
}

void FileOperationState::clear() {
    kind_ = PendingFileOperationKind::None;
    sourcePaths_.clear();
}

void FileOperationState::markPasteSucceeded() {
    if (kind_ == PendingFileOperationKind::Cut) {
        clear();
    }
}

bool FileOperationState::hasPendingOperation() const {
    return kind_ != PendingFileOperationKind::None && !sourcePaths_.empty();
}

PendingFileOperationKind FileOperationState::kind() const {
    return kind_;
}

const std::wstring& FileOperationState::sourcePath() const {
    static const std::wstring empty;
    if (sourcePaths_.empty()) {
        return empty;
    }

    return sourcePaths_.front();
}

const std::vector<std::wstring>& FileOperationState::sourcePaths() const {
    return sourcePaths_;
}

} // namespace finderx::ui
