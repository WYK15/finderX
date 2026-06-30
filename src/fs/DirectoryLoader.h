#pragma once

#include "fs/FileMetadata.h"
#include "model/FileNode.h"

#include <string>
#include <vector>

namespace finderx {

class DirectoryLoader {
public:
    std::vector<FileNode> loadChildren(const std::wstring& directoryPath) const;
};

std::wstring defaultHomeDirectory();
std::wstring fileNameFromPath(const std::wstring& path);

}
