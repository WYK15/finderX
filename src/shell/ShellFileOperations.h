#pragma once

#include <windows.h>

#include <string>

namespace finderx::shell {

struct FileOperationResult {
    bool success = false;
    std::wstring message;
    std::wstring resultingPath;
};

FileOperationResult renamePath(HWND owner, const std::wstring& oldPath, const std::wstring& newName);
FileOperationResult moveToTrash(HWND owner, const std::wstring& path);
FileOperationResult copyToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory);
FileOperationResult moveToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory);

} // namespace finderx::shell
