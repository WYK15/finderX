#pragma once

#include "fs/FileMetadata.h"
#include "model/FileNode.h"

#include <string>
#include <vector>

namespace finderx {

struct DirectoryLoadResult {
    std::vector<FileNode> children;
    DWORD error = ERROR_SUCCESS;

    bool ok() const;
};

class DirectoryLoader {
public:
    std::vector<FileNode> loadChildren(const std::wstring& directoryPath, bool includeHiddenAndSystem = false) const;
    DirectoryLoadResult loadChildrenWithStatus(const std::wstring& directoryPath, bool includeHiddenAndSystem = false) const;
};

std::wstring defaultHomeDirectory();
std::wstring fileNameFromPath(const std::wstring& path);

}
