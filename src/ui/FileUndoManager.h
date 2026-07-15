#pragma once

#include <span>
#include <string>
#include <vector>

namespace finderx::ui {

enum class UndoFileOperationKind {
    None,
    CreatedPaths,
    RenamedPath,
    MovedPaths
};

struct UndoFileOperation {
    UndoFileOperationKind kind = UndoFileOperationKind::None;
    std::vector<std::wstring> sourcePaths;
    std::vector<std::wstring> destinationPaths;
};

class FileUndoManager {
public:
    void recordCreatedPath(std::wstring path);
    void recordCreatedPaths(std::vector<std::wstring> paths);
    void recordRename(std::wstring oldPath, std::wstring newPath);
    void recordMove(std::vector<std::wstring> sourcePaths, std::vector<std::wstring> destinationPaths);
    bool hasUndo() const;
    const UndoFileOperation& lastOperation() const;
    void clear();

private:
    UndoFileOperation lastOperation_;
};

std::vector<std::wstring> pathsInDirectory(std::span<const std::wstring> sourcePaths, const std::wstring& directory);

} // namespace finderx::ui
