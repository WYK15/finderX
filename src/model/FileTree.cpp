#include "model/FileTree.h"

#include <stdexcept>
#include <utility>

namespace finderx {

FileTree::FileTree() {
    nodes_.push_back(FileNode{});
    rootId_ = addNode(kInvalidNodeId, L"home", FileKind::Folder, L"", L"--", L"\u6587\u4ef6\u5939");
    nodes_[rootId_].expanded = true;
}

NodeId FileTree::rootId() const {
    return rootId_;
}

const FileNode& FileTree::node(NodeId id) const {
    if (id == kInvalidNodeId || id >= nodes_.size()) {
        throw std::out_of_range("invalid node id");
    }
    return nodes_[id];
}

FileNode& FileTree::node(NodeId id) {
    if (id == kInvalidNodeId || id >= nodes_.size()) {
        throw std::out_of_range("invalid node id");
    }
    return nodes_[id];
}

std::span<const FileNode> FileTree::nodes() const {
    return nodes_;
}

NodeId FileTree::addNode(NodeId parent, std::wstring name, FileKind kind,
                         std::wstring modified, std::wstring size, std::wstring kindText) {
    if (parent != kInvalidNodeId && node(parent).kind != FileKind::Folder) {
        throw std::invalid_argument("parent must be a folder");
    }

    const NodeId id = static_cast<NodeId>(nodes_.size());
    FileNode item;
    item.id = id;
    item.parent = parent;
    item.name = std::move(name);
    item.modified = std::move(modified);
    item.size = std::move(size);
    item.kindText = std::move(kindText);
    item.kind = kind;
    nodes_.push_back(std::move(item));

    if (parent != kInvalidNodeId) {
        node(parent).children.push_back(id);
    }

    return id;
}

void FileTree::setExpanded(NodeId id, bool expanded) {
    FileNode& item = node(id);
    if (item.kind == FileKind::Folder) {
        item.expanded = expanded;
    }
}

void FileTree::toggleExpanded(NodeId id) {
    FileNode& item = node(id);
    if (item.kind == FileKind::Folder) {
        item.expanded = !item.expanded;
    }
}

std::vector<VisibleRow> FileTree::flatten() const {
    std::vector<VisibleRow> rows;
    for (NodeId child : node(rootId_).children) {
        flattenFrom(child, 0, rows);
    }
    return rows;
}

void FileTree::flattenFrom(NodeId id, int depth, std::vector<VisibleRow>& rows) const {
    const FileNode& item = node(id);
    rows.push_back(VisibleRow{id, depth});

    if (item.kind != FileKind::Folder || !item.expanded) {
        return;
    }

    for (NodeId child : item.children) {
        flattenFrom(child, depth + 1, rows);
    }
}

FileTree FileTree::sample() {
    FileTree tree;

    NodeId environments = tree.addNode(tree.rootId(), L"environments", FileKind::Folder,
                                       L"2025\u5e747\u670828\u65e5 22:32", L"--", L"\u6587\u4ef6\u5939");
    NodeId androidEnv = tree.addNode(environments, L"androidEnv", FileKind::Folder,
                                     L"2025\u5e747\u670828\u65e5 22:32", L"--", L"\u6587\u4ef6\u5939");
    NodeId platformTools = tree.addNode(androidEnv, L"platform-tools", FileKind::Folder,
                                        L"2025\u5e747\u670828\u65e5 22:32", L"--", L"\u6587\u4ef6\u5939");

    tree.addNode(platformTools, L"adb", FileKind::File,
                 L"2025\u5e744\u670824\u65e5 02:33", L"15.6 MB", L"Unix \u53ef\u6267\u884c\u6587\u4ef6");
    tree.addNode(platformTools, L"etc1tool", FileKind::File,
                 L"2025\u5e744\u670824\u65e5 02:33", L"688 KB", L"Unix \u53ef\u6267\u884c\u6587\u4ef6");
    tree.addNode(platformTools, L"fastboot", FileKind::File,
                 L"2025\u5e744\u670824\u65e5 02:33", L"4.8 MB", L"Unix \u53ef\u6267\u884c\u6587\u4ef6");
    tree.addNode(platformTools, L"hprof-conv", FileKind::File,
                 L"2025\u5e744\u670824\u65e5 02:33", L"135 KB", L"Unix \u53ef\u6267\u884c\u6587\u4ef6");
    tree.addNode(platformTools, L"NOTICE.txt", FileKind::File,
                 L"2008\u5e741\u67081\u65e5 15:00", L"1.1 MB", L"\u7eaf\u6587\u672c\u6587\u7a3f");
    tree.addNode(androidEnv, L"lib64", FileKind::Folder,
                 L"2025\u5e744\u670824\u65e5 02:33", L"--", L"\u6587\u4ef6\u5939");

    tree.addNode(tree.rootId(), L"debugserver-16.5", FileKind::Folder,
                 L"2025\u5e747\u670826\u65e5 23:54", L"--", L"\u6587\u4ef6\u5939");
    tree.addNode(tree.rootId(), L"MonkeyDev", FileKind::Folder,
                 L"2024\u5e746\u670818\u65e5 21:50", L"--", L"\u6587\u4ef6\u5939");

    NodeId projects = tree.addNode(tree.rootId(), L"projects", FileKind::Folder,
                                   L"2026\u5e746\u67084\u65e5 23:01", L"--", L"\u6587\u4ef6\u5939");
    tree.addNode(projects, L"4utool", FileKind::Folder,
                 L"2026\u5e745\u670830\u65e5 00:02", L"--", L"\u6587\u4ef6\u5939");
    tree.addNode(projects, L"codexopen", FileKind::Folder,
                 L"2026\u5e745\u670829\u65e5 23:57", L"--", L"\u6587\u4ef6\u5939");
    tree.addNode(projects, L"readme.md", FileKind::File,
                 L"2026\u5e745\u670829\u65e5 23:57", L"551 \u5b57\u8282", L"Markdown Text");

    tree.setExpanded(environments, true);
    tree.setExpanded(androidEnv, true);
    tree.setExpanded(platformTools, true);
    tree.setExpanded(projects, true);

    return tree;
}

}
