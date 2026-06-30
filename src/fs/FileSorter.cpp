#include "fs/FileSorter.h"

#include <algorithm>
#include <cwchar>

namespace finderx {
namespace {

int compareCaseInsensitive(const std::wstring& left, const std::wstring& right) {
    return _wcsicmp(left.c_str(), right.c_str());
}

int compareUnsigned(unsigned long long left, unsigned long long right) {
    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

int compareSelectedColumn(const FileNode& left, const FileNode& right, SortColumn column) {
    switch (column) {
    case SortColumn::Modified:
        return compareUnsigned(left.modifiedTicks, right.modifiedTicks);
    case SortColumn::Size:
        return compareUnsigned(left.sizeBytes, right.sizeBytes);
    case SortColumn::Kind:
        return compareCaseInsensitive(left.kindText, right.kindText);
    case SortColumn::Name:
    default:
        return compareCaseInsensitive(left.name, right.name);
    }
}

} // namespace

void sortFileNodes(std::vector<FileNode>& nodes, SortColumn column, SortDirection direction) {
    std::stable_sort(nodes.begin(), nodes.end(), [&](const FileNode& left, const FileNode& right) {
        if (left.kind != right.kind) {
            return left.kind == FileKind::Folder;
        }

        int selectedComparison = compareSelectedColumn(left, right, column);
        if (selectedComparison != 0) {
            return direction == SortDirection::Ascending ? selectedComparison < 0 : selectedComparison > 0;
        }

        return compareCaseInsensitive(left.name, right.name) < 0;
    });
}

} // namespace finderx
