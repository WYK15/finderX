#pragma once

#include <string>

namespace finderx {

struct FileCreationResult {
    bool success = false;
    std::wstring path;
    std::wstring message;
};

std::wstring nextNewFolderPath(const std::wstring& directory);
std::wstring nextNewTextFilePath(const std::wstring& directory);
FileCreationResult createNewFolder(const std::wstring& directory);
FileCreationResult createNewTextFile(const std::wstring& directory);

}
