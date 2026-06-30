#include "shell/ShellFileOperations.h"

#include "ui/RenameDialog.h"

#include <shellapi.h>

#include <filesystem>
#include <span>
#include <system_error>
#include <utility>

namespace finderx::shell {
namespace {

std::wstring doubleNullTerminated(std::span<const std::wstring> paths) {
    std::wstring buffer;
    for (const std::wstring& path : paths) {
        buffer.append(path);
        buffer.push_back(L'\0');
    }
    buffer.push_back(L'\0');
    return buffer;
}

FileOperationResult failed(std::wstring message) {
    FileOperationResult result;
    result.message = std::move(message);
    return result;
}

bool isAbsolutePath(const std::wstring& path) {
    return std::filesystem::path(path).is_absolute();
}

bool allAbsolute(std::span<const std::wstring> paths) {
    if (paths.empty()) {
        return false;
    }

    for (const std::wstring& path : paths) {
        if (path.empty() || !isAbsolutePath(path)) {
            return false;
        }
    }

    return true;
}

FileOperationResult mapFailureMessage(
    FileOperationResult result,
    const std::wstring& batchMessage,
    const std::wstring& singleMessage
) {
    if (!result.success && result.message == batchMessage) {
        result.message = singleMessage;
    }

    return result;
}

FileOperationResult runShellOperation(
    HWND owner,
    UINT function,
    std::span<const std::wstring> sourcePaths,
    const std::wstring& destinationDirectory,
    const std::wstring& failureMessage
) {
    if (!allAbsolute(sourcePaths)) {
        return failed(failureMessage);
    }

    std::wstring sourceBuffer = doubleNullTerminated(sourcePaths);
    std::wstring destinationBuffer;
    if (!destinationDirectory.empty()) {
        destinationBuffer = doubleNullTerminated(std::span<const std::wstring>(&destinationDirectory, 1));
    }

    SHFILEOPSTRUCTW operation = {};
    operation.hwnd = owner;
    operation.wFunc = function;
    operation.pFrom = sourceBuffer.c_str();
    operation.pTo = destinationBuffer.empty() ? nullptr : destinationBuffer.c_str();
    operation.fFlags = FOF_ALLOWUNDO;

    const int operationResult = SHFileOperationW(&operation);
    if (operationResult != 0 || operation.fAnyOperationsAborted) {
        return failed(failureMessage);
    }

    FileOperationResult result;
    result.success = true;
    return result;
}

} // namespace

FileOperationResult renamePath(HWND, const std::wstring& oldPath, const std::wstring& newName) {
    constexpr wchar_t failureMessage[] = L"Cannot rename item";

    if (oldPath.empty() || !isAbsolutePath(oldPath) || !ui::isValidRenameName(newName)) {
        return failed(failureMessage);
    }

    const std::filesystem::path sourcePath(oldPath);
    const std::filesystem::path targetPath = sourcePath.parent_path() / newName;

    std::error_code error;
    std::filesystem::rename(sourcePath, targetPath, error);
    if (error) {
        return failed(failureMessage);
    }

    FileOperationResult result;
    result.success = true;
    result.resultingPath = targetPath.wstring();
    return result;
}

FileOperationResult moveToTrash(HWND owner, const std::wstring& path) {
    return mapFailureMessage(
        moveToTrash(owner, std::span<const std::wstring>(&path, 1)),
        L"Cannot move items to trash",
        L"Cannot move item to trash"
    );
}

FileOperationResult moveToTrash(HWND owner, std::span<const std::wstring> paths) {
    return runShellOperation(owner, FO_DELETE, paths, L"", L"Cannot move items to trash");
}

FileOperationResult copyToDirectory(
    HWND owner,
    const std::wstring& sourcePath,
    const std::wstring& destinationDirectory
) {
    return mapFailureMessage(
        copyToDirectory(owner, std::span<const std::wstring>(&sourcePath, 1), destinationDirectory),
        L"Cannot copy items",
        L"Cannot copy item"
    );
}

FileOperationResult copyToDirectory(
    HWND owner,
    std::span<const std::wstring> sourcePaths,
    const std::wstring& destinationDirectory
) {
    if (destinationDirectory.empty()) {
        return failed(L"Cannot paste here");
    }

    if (!isAbsolutePath(destinationDirectory)) {
        return failed(L"Cannot copy items");
    }

    return runShellOperation(owner, FO_COPY, sourcePaths, destinationDirectory, L"Cannot copy items");
}

FileOperationResult moveToDirectory(
    HWND owner,
    const std::wstring& sourcePath,
    const std::wstring& destinationDirectory
) {
    return mapFailureMessage(
        moveToDirectory(owner, std::span<const std::wstring>(&sourcePath, 1), destinationDirectory),
        L"Cannot move items",
        L"Cannot move item"
    );
}

FileOperationResult moveToDirectory(
    HWND owner,
    std::span<const std::wstring> sourcePaths,
    const std::wstring& destinationDirectory
) {
    if (destinationDirectory.empty()) {
        return failed(L"Cannot paste here");
    }

    if (!isAbsolutePath(destinationDirectory)) {
        return failed(L"Cannot move items");
    }

    return runShellOperation(owner, FO_MOVE, sourcePaths, destinationDirectory, L"Cannot move items");
}

} // namespace finderx::shell
