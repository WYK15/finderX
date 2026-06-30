#pragma once

#include "model/FileNode.h"

#include <span>
#include <vector>

namespace finderx {

class FileTree {
public:
    FileTree();
    explicit FileTree(std::wstring rootPath, std::wstring rootName);

    NodeId rootId() const;
    const FileNode& node(NodeId id) const;
    FileNode& node(NodeId id);
    std::span<const FileNode> nodes() const;

    NodeId addNode(NodeId parent, std::wstring name, std::wstring path, FileKind kind,
                   std::wstring modified, std::wstring size, std::wstring kindText);
    NodeId addNode(NodeId parent, std::wstring name, FileKind kind,
                   std::wstring modified, std::wstring size, std::wstring kindText);
    void replaceChildren(NodeId parent, std::vector<FileNode> children);
    void setExpanded(NodeId id, bool expanded);
    void setChildrenLoaded(NodeId id, bool loaded);
    void toggleExpanded(NodeId id);
    std::vector<VisibleRow> flatten() const;

    static FileTree sample();

private:
    void detachSubtree(NodeId id);
    void flattenFrom(NodeId id, int depth, std::vector<VisibleRow>& rows) const;

    std::vector<FileNode> nodes_;
    NodeId rootId_ = kInvalidNodeId;
};

}
