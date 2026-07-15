#include "ui/FileUndoManager.h"

#include <filesystem>
#include <utility>

namespace finderx::ui {

namespace {

std::vector<std::wstring> nonEmptyPaths(std::vector<std::wstring> paths) {
    std::vector<std::wstring> cleaned;
    cleaned.reserve(paths.size());
    for (std::wstring& path : paths) {
        if (!path.empty()) {
            cleaned.push_back(std::move(path));
        }
    }
    return cleaned;
}

} // namespace

void FileUndoManager::recordCreatedPath(std::wstring path) {
    std::vector<std::wstring> paths;
    paths.push_back(std::move(path));
    recordCreatedPaths(std::move(paths));
}

void FileUndoManager::recordCreatedPaths(std::vector<std::wstring> paths) {
    paths = nonEmptyPaths(std::move(paths));
    if (paths.empty()) {
        clear();
        return;
    }

    lastOperation_ = {};
    lastOperation_.kind = UndoFileOperationKind::CreatedPaths;
    lastOperation_.destinationPaths = std::move(paths);
}

void FileUndoManager::recordRename(std::wstring oldPath, std::wstring newPath) {
    if (oldPath.empty() || newPath.empty()) {
        clear();
        return;
    }

    lastOperation_ = {};
    lastOperation_.kind = UndoFileOperationKind::RenamedPath;
    lastOperation_.sourcePaths.push_back(std::move(oldPath));
    lastOperation_.destinationPaths.push_back(std::move(newPath));
}

void FileUndoManager::recordMove(std::vector<std::wstring> sourcePaths, std::vector<std::wstring> destinationPaths) {
    sourcePaths = nonEmptyPaths(std::move(sourcePaths));
    destinationPaths = nonEmptyPaths(std::move(destinationPaths));
    if (sourcePaths.empty() || sourcePaths.size() != destinationPaths.size()) {
        clear();
        return;
    }

    lastOperation_ = {};
    lastOperation_.kind = UndoFileOperationKind::MovedPaths;
    lastOperation_.sourcePaths = std::move(sourcePaths);
    lastOperation_.destinationPaths = std::move(destinationPaths);
}

bool FileUndoManager::hasUndo() const {
    return lastOperation_.kind != UndoFileOperationKind::None;
}

const UndoFileOperation& FileUndoManager::lastOperation() const {
    return lastOperation_;
}

void FileUndoManager::clear() {
    lastOperation_ = {};
}

std::vector<std::wstring> pathsInDirectory(std::span<const std::wstring> sourcePaths, const std::wstring& directory) {
    std::vector<std::wstring> paths;
    paths.reserve(sourcePaths.size());
    const std::filesystem::path destinationDirectory(directory);
    for (const std::wstring& sourcePath : sourcePaths) {
        const std::filesystem::path source(sourcePath);
        const std::filesystem::path filename = source.filename();
        if (destinationDirectory.empty() || filename.empty()) {
            paths.push_back(L"");
        } else {
            paths.push_back((destinationDirectory / filename).wstring());
        }
    }
    return paths;
}

} // namespace finderx::ui
