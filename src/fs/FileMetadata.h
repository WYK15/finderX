#pragma once

#include "model/FileNode.h"

#include <windows.h>

#include <string>
#include <string_view>

namespace finderx {

struct FileMetadata {
    std::wstring name;
    std::wstring path;
    FileKind kind = FileKind::File;
    std::wstring modified;
    std::wstring size;
    std::wstring kindText;
};

std::wstring formatFileSize(unsigned long long bytes, bool isDirectory);
std::wstring formatFileTime(const FILETIME& fileTime);
std::wstring kindTextForAttributes(DWORD attributes);
bool isSkippableDirectoryEntry(std::wstring_view name);

}
