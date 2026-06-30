#include "shell/ShellFileOperations.h"

#include "ui/RenameDialog.h"

#include <shellapi.h>

#include <filesystem>
#include <system_error>
#include <utility>
#include <vector>

namespace finderx::shell {
namespace {

std::vector<wchar_t> makeShellPathBuffer(const std::wstring& path) {
    std::vector<wchar_t> buffer(path.begin(), path.end());
    buffer.push_back(L'\0');
    buffer.push_back(L'\0');
    return buffer;
}

FileOperationResult failed(std::wstring message) {
    FileOperationResult result;
    result.message = std::move(message);
    return result;
}

FileOperationResult runShellOperation(
    HWND owner,
    UINT function,
    const std::wstring& sourcePath,
    const std::wstring& destinationDirectory,
    const std::wstring& failureMessage
) {
    if (sourcePath.empty()) {
        return failed(failureMessage);
    }

    std::vector<wchar_t> sourceBuffer = makeShellPathBuffer(sourcePath);
    std::vector<wchar_t> destinationBuffer;
    if (!destinationDirectory.empty()) {
        destinationBuffer = makeShellPathBuffer(destinationDirectory);
    }

    SHFILEOPSTRUCTW operation = {};
    operation.hwnd = owner;
    operation.wFunc = function;
    operation.pFrom = sourceBuffer.data();
    operation.pTo = destinationBuffer.empty() ? nullptr : destinationBuffer.data();
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

    if (oldPath.empty() || !ui::isValidRenameName(newName)) {
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
    constexpr wchar_t failureMessage[] = L"Cannot move item to trash";

    if (path.empty() || !std::filesystem::path(path).is_absolute()) {
        return failed(failureMessage);
    }

    return runShellOperation(owner, FO_DELETE, path, L"", failureMessage);
}

FileOperationResult copyToDirectory(
    HWND owner,
    const std::wstring& sourcePath,
    const std::wstring& destinationDirectory
) {
    if (destinationDirectory.empty()) {
        return failed(L"Cannot paste here");
    }

    return runShellOperation(owner, FO_COPY, sourcePath, destinationDirectory, L"Cannot copy item");
}

FileOperationResult moveToDirectory(
    HWND owner,
    const std::wstring& sourcePath,
    const std::wstring& destinationDirectory
) {
    if (destinationDirectory.empty()) {
        return failed(L"Cannot paste here");
    }

    return runShellOperation(owner, FO_MOVE, sourcePath, destinationDirectory, L"Cannot move item");
}

} // namespace finderx::shell
