#pragma once

#include <windows.h>

#include <span>
#include <string>

namespace finderx::shell {

struct FileOperationResult {
    bool success = false;
    std::wstring message;
    std::wstring resultingPath;
};

FileOperationResult renamePath(HWND owner, const std::wstring& oldPath, const std::wstring& newName);
FileOperationResult moveToTrash(HWND owner, const std::wstring& path);
FileOperationResult moveToTrash(HWND owner, std::span<const std::wstring> paths);
FileOperationResult copyToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory);
FileOperationResult copyToDirectory(
    HWND owner,
    std::span<const std::wstring> sourcePaths,
    const std::wstring& destinationDirectory
);
FileOperationResult moveToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory);
FileOperationResult moveToDirectory(
    HWND owner,
    std::span<const std::wstring> sourcePaths,
    const std::wstring& destinationDirectory
);

} // namespace finderx::shell
