#pragma once

#include "model/FileNode.h"
#include "settings/AppSettings.h"

#include <vector>

namespace finderx {

void sortFileNodes(std::vector<FileNode>& nodes, SortColumn column, SortDirection direction);

} // namespace finderx
