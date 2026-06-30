#include "fs/FileCreation.h"

#include <windows.h>

namespace finderx {
namespace {

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

std::wstring nextAvailablePath(const std::wstring& directory,
                               const std::wstring& base,
                               const std::wstring& extension) {
    for (int index = 1; index <= 9999; ++index) {
        const std::wstring path = joinPath(directory, numberedName(base, extension, index));
        if (!pathExists(path)) {
            return path;
        }
    }

    return L"";
}

}

std::wstring nextNewFolderPath(const std::wstring& directory) {
    return nextAvailablePath(directory, L"New Folder", L"");
}

std::wstring nextNewTextFilePath(const std::wstring& directory) {
    return nextAvailablePath(directory, L"New Text Document", L".txt");
}

FileCreationResult createNewFolder(const std::wstring& directory) {
    FileCreationResult result;
    result.path = nextNewFolderPath(directory);
    if (!result.path.empty() && CreateDirectoryW(result.path.c_str(), nullptr)) {
        result.success = true;
        return result;
    }

    result.message = L"Cannot create folder";
    return result;
}

FileCreationResult createNewTextFile(const std::wstring& directory) {
    FileCreationResult result;
    result.path = nextNewTextFilePath(directory);
    if (result.path.empty()) {
        result.message = L"Cannot create file";
        return result;
    }

    HANDLE handle = CreateFileW(result.path.c_str(),
                                GENERIC_WRITE,
                                0,
                                nullptr,
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        result.message = L"Cannot create file";
        return result;
    }

    CloseHandle(handle);
    result.success = true;
    return result;
}

}
