#pragma once

#include "model/FileNode.h"

#include <string>
#include <vector>

namespace finderx {

std::vector<std::wstring> enumerateDriveRoots();
std::vector<FileNode> enumerateDriveNodes();
std::vector<std::wstring> driveRootsFromMaskForTests(unsigned long mask);
std::vector<FileNode> driveNodesFromRootsForTests(const std::vector<std::wstring>& roots);

} // namespace finderx
