#include "model/FileTree.h"

#include <cstdlib>
#include <stdexcept>
#include <iostream>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

int main() {
    FileTree tree = FileTree::sample();
    auto rows = tree.flatten();
    require(!rows.empty(), "sample tree should flatten to visible rows");
    require(tree.node(rows.front().nodeId).name == L"environments", "first visible row should be environments");

    const std::size_t expandedCount = rows.size();
    tree.toggleExpanded(rows.front().nodeId);
    rows = tree.flatten();
    require(rows.size() < expandedCount, "collapsing environments should reduce visible row count");

    tree.toggleExpanded(rows.front().nodeId);
    rows = tree.flatten();
    require(rows.size() == expandedCount, "expanding environments should restore visible row count");

    const NodeId file = tree.addNode(tree.rootId(), L"not-a-folder.txt", FileKind::File, L"", L"0 bytes", L"Text");
    bool rejectedNonFolderParent = false;
    try {
        tree.addNode(file, L"child", FileKind::File, L"", L"0 bytes", L"Text");
    } catch (const std::invalid_argument&) {
        rejectedNonFolderParent = true;
    }
    require(rejectedNonFolderParent, "adding a child to a non-folder parent should throw invalid_argument");

    std::cout << "FileTreeTests passed\n";
    return 0;
}
