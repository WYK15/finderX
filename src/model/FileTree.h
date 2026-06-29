#pragma once

#include "model/FileNode.h"

#include <span>
#include <vector>

namespace finderx {

class FileTree {
public:
    FileTree();

    NodeId rootId() const;
    const FileNode& node(NodeId id) const;
    FileNode& node(NodeId id);
    std::span<const FileNode> nodes() const;

    NodeId addNode(NodeId parent, std::wstring name, FileKind kind,
                   std::wstring modified, std::wstring size, std::wstring kindText);
    void setExpanded(NodeId id, bool expanded);
    void toggleExpanded(NodeId id);
    std::vector<VisibleRow> flatten() const;

    static FileTree sample();

private:
    void flattenFrom(NodeId id, int depth, std::vector<VisibleRow>& rows) const;

    std::vector<FileNode> nodes_;
    NodeId rootId_ = kInvalidNodeId;
};

}
