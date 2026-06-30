#include "fs/DirectoryLoader.h"

#include <windows.h>

#include <algorithm>
#include <cwchar>

namespace finderx {
namespace {

unsigned long long fileSizeFromFindData(const WIN32_FIND_DATAW& data) {
    ULARGE_INTEGER value{};
    value.HighPart = data.nFileSizeHigh;
    value.LowPart = data.nFileSizeLow;
    return value.QuadPart;
}

std::wstring joinSearchPattern(const std::wstring& directoryPath) {
    if (directoryPath.empty()) {
        return L"*";
    }

    const wchar_t last = directoryPath.back();
    if (last == L'\\' || last == L'/') {
        return directoryPath + L"*";
    }
    return directoryPath + L"\\*";
}

std::wstring joinPath(const std::wstring& directoryPath, const std::wstring& name) {
    if (directoryPath.empty()) {
        return name;
    }

    const wchar_t last = directoryPath.back();
    if (last == L'\\' || last == L'/') {
        return directoryPath + name;
    }
    return directoryPath + L"\\" + name;
}

std::wstring environmentVariable(const wchar_t* name) {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(name, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return L"";
    }
    return std::wstring(buffer, length);
}

} // namespace

std::vector<FileNode> DirectoryLoader::loadChildren(const std::wstring& directoryPath) const {
    std::vector<FileNode> nodes;
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = joinSearchPattern(directoryPath);
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return nodes;
    }

    do {
        const std::wstring name = data.cFileName;
        if (isSkippableDirectoryEntry(name)) {
            continue;
        }

        const bool isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        FileNode node;
        node.name = name;
        node.path = joinPath(directoryPath, name);
        node.kind = isDirectory ? FileKind::Folder : FileKind::File;
        node.modified = formatFileTime(data.ftLastWriteTime);
        node.size = formatFileSize(fileSizeFromFindData(data), isDirectory);
        node.kindText = kindTextForAttributes(data.dwFileAttributes);
        node.childrenLoaded = false;
        nodes.push_back(std::move(node));
    } while (FindNextFileW(find, &data));

    FindClose(find);

    std::stable_sort(nodes.begin(), nodes.end(), [](const FileNode& left, const FileNode& right) {
        if (left.kind != right.kind) {
            return left.kind == FileKind::Folder;
        }
        return _wcsicmp(left.name.c_str(), right.name.c_str()) < 0;
    });

    return nodes;
}

std::wstring defaultHomeDirectory() {
    std::wstring home = environmentVariable(L"USERPROFILE");
    if (!home.empty()) {
        return home;
    }

    home = environmentVariable(L"HOMEPATH");
    if (!home.empty()) {
        return home;
    }

    return L"C:\\";
}

std::wstring fileNameFromPath(const std::wstring& path) {
    std::size_t end = path.find_last_not_of(L"\\/");
    if (end == std::wstring::npos) {
        return L"";
    }

    const std::size_t slash = path.find_last_of(L"\\/", end);
    if (slash == std::wstring::npos) {
        return path.substr(0, end + 1);
    }
    return path.substr(slash + 1, end - slash);
}

}
