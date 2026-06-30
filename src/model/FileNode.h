#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace finderx {

using NodeId = std::uint32_t;
inline constexpr NodeId kInvalidNodeId = 0;

enum class FileKind {
    Folder,
    File,
};

struct FileNode {
    NodeId id = kInvalidNodeId;
    NodeId parent = kInvalidNodeId;
    std::wstring name;
    std::wstring path;
    std::wstring modified;
    std::wstring size;
    std::wstring kindText;
    FileKind kind = FileKind::File;
    bool expanded = false;
    bool childrenLoaded = false;
    std::vector<NodeId> children;
};

struct VisibleRow {
    NodeId nodeId = kInvalidNodeId;
    int depth = 0;
};

}
