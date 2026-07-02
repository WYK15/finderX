#include "fs/ZipOperations.h"

#include <algorithm>
#include <filesystem>

#include <windows.h>

namespace finderx {
namespace {

std::wstring lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t character) {
        return static_cast<wchar_t>(towlower(character));
    });
    return value;
}

std::wstring joinPath(const std::wstring& directory, const std::wstring& child) {
    if (directory.empty() || directory.back() == L'\\' || directory.back() == L'/') {
        return directory + child;
    }
    return directory + L"\\" + child;
}

bool pathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring numberedName(const std::wstring& base, const std::wstring& extension, int index) {
    if (index == 1) {
        return base + extension;
    }
    return base + L" " + std::to_wstring(index) + extension;
}

std::wstring nextAvailablePath(const std::wstring& directory, const std::wstring& base, const std::wstring& extension) {
    for (int index = 1; index <= 9999; ++index) {
        const std::wstring candidate = joinPath(directory, numberedName(base, extension, index));
        if (!pathExists(candidate)) {
            return candidate;
        }
    }
    return L"";
}

std::wstring fileStem(const std::wstring& path) {
    std::filesystem::path fsPath(path);
    std::wstring stem = fsPath.stem().wstring();
    if (stem.empty()) {
        stem = fsPath.filename().wstring();
    }
    if (stem.empty()) {
        stem = L"Archive";
    }
    return stem;
}

std::wstring quoteArgument(const std::wstring& argument) {
    std::wstring quoted = L"\"";
    unsigned int backslashes = 0;
    for (wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(character);
            backslashes = 0;
            continue;
        }
        quoted.append(backslashes, L'\\');
        backslashes = 0;
        quoted.push_back(character);
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

bool runProcess(const std::wstring& commandLine, const std::wstring& workingDirectory) {
    std::wstring mutableCommand = commandLine;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(
        nullptr,
        mutableCommand.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startup,
        &process);
    if (!created) {
        return false;
    }

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return exitCode == 0;
}

std::wstring relativePathForTar(const std::wstring& currentDirectory, const std::wstring& sourcePath) {
    std::error_code error;
    std::filesystem::path relative = std::filesystem::relative(sourcePath, currentDirectory, error);
    if (error || relative.empty()) {
        relative = std::filesystem::path(sourcePath).filename();
    }
    return relative.wstring();
}

} // namespace

bool isZipArchivePath(const std::wstring& path) {
    return lowercase(std::filesystem::path(path).extension().wstring()) == L".zip";
}

std::wstring defaultZipArchivePath(const std::wstring& currentDirectory, std::span<const std::wstring> sourcePaths) {
    if (currentDirectory.empty() || sourcePaths.empty()) {
        return L"";
    }
    const std::wstring base = sourcePaths.size() == 1 ? fileStem(sourcePaths.front()) : L"Archive";
    return nextAvailablePath(currentDirectory, base, L".zip");
}

std::wstring defaultZipExtractDirectory(const std::wstring& currentDirectory, const std::wstring& zipPath) {
    if (currentDirectory.empty() || zipPath.empty()) {
        return L"";
    }
    return nextAvailablePath(currentDirectory, fileStem(zipPath), L"");
}

ZipOperationResult compressToZip(const std::wstring& currentDirectory, std::span<const std::wstring> sourcePaths) {
    ZipOperationResult result;
    result.outputPath = defaultZipArchivePath(currentDirectory, sourcePaths);
    if (result.outputPath.empty() || sourcePaths.empty()) {
        result.message = L"Cannot create zip archive";
        return result;
    }

    std::wstring command = L"tar.exe -a -cf " + quoteArgument(result.outputPath) + L" --";
    for (const std::wstring& sourcePath : sourcePaths) {
        command += L" " + quoteArgument(relativePathForTar(currentDirectory, sourcePath));
    }

    if (!runProcess(command, currentDirectory)) {
        result.message = L"Cannot create zip archive";
        return result;
    }
    result.success = true;
    return result;
}

ZipOperationResult extractZipHere(const std::wstring& currentDirectory, const std::wstring& zipPath) {
    ZipOperationResult result;
    result.outputPath = defaultZipExtractDirectory(currentDirectory, zipPath);
    if (result.outputPath.empty() || !isZipArchivePath(zipPath)) {
        result.message = L"Cannot extract zip archive";
        return result;
    }
    if (!CreateDirectoryW(result.outputPath.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        result.message = L"Cannot create extraction folder";
        return result;
    }

    const std::wstring command = L"tar.exe -xf " + quoteArgument(zipPath) + L" -C " + quoteArgument(result.outputPath);
    if (!runProcess(command, currentDirectory)) {
        result.message = L"Cannot extract zip archive";
        return result;
    }
    result.success = true;
    return result;
}

} // namespace finderx
