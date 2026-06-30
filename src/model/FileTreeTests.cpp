#include "model/FileTree.h"

#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include <utility>

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

    FileTree realTree(L"C:\\Users\\Example", L"Example");
    require(realTree.node(realTree.rootId()).path == L"C:\\Users\\Example", "root path should be stored");

    std::vector<FileNode> children;
    FileNode child;
    child.name = L"Documents";
    child.path = L"C:\\Users\\Example\\Documents";
    child.kind = FileKind::Folder;
    child.size = L"--";
    child.kindText = L"Folder";
    child.modifiedTicks = 123456789;
    child.sizeBytes = 4096;
    children.push_back(std::move(child));

    realTree.replaceChildren(realTree.rootId(), std::move(children));
    require(realTree.node(realTree.rootId()).childrenLoaded, "replaceChildren should mark loaded");
    auto realRows = realTree.flatten();
    require(realRows.size() == 1, "real tree should flatten replaced child");
    const NodeId oldChild = realRows.front().nodeId;
    require(realTree.node(oldChild).path == L"C:\\Users\\Example\\Documents", "child path should be stored");
    require(realTree.node(oldChild).modifiedTicks == 123456789, "child modified ticks should be preserved");
    require(realTree.node(oldChild).sizeBytes == 4096, "child size bytes should be preserved");

    const NodeId oldGrandchild = realTree.addNode(oldChild, L"Nested", L"C:\\Users\\Example\\Documents\\Nested",
                                                  FileKind::Folder, L"", L"--", L"Folder");
    realTree.setExpanded(oldChild, true);
    realTree.setChildrenLoaded(oldChild, true);

    std::vector<FileNode> replacementChildren;
    FileNode replacementChild;
    replacementChild.name = L"Desktop";
    replacementChild.path = L"C:\\Users\\Example\\Desktop";
    replacementChild.kind = FileKind::Folder;
    replacementChild.size = L"--";
    replacementChild.kindText = L"Folder";
    replacementChildren.push_back(std::move(replacementChild));

    realTree.replaceChildren(realTree.rootId(), std::move(replacementChildren));
    require(realTree.node(oldChild).parent == kInvalidNodeId, "replaced child should be detached from parent");
    require(realTree.node(oldChild).children.empty(), "detached child should not retain children");
    require(!realTree.node(oldChild).expanded, "detached child should be collapsed");
    require(!realTree.node(oldChild).childrenLoaded, "detached child should not remain loaded");
    require(realTree.node(oldGrandchild).parent == kInvalidNodeId, "detached grandchild should be detached");
    realRows = realTree.flatten();
    require(realRows.size() == 1, "real tree should only flatten replacement child");
    require(realTree.node(realRows.front().nodeId).name == L"Desktop", "replacement child should be visible");

    realTree.setChildrenLoaded(realTree.rootId(), false);
    require(!realTree.node(realTree.rootId()).childrenLoaded, "setChildrenLoaded should update loaded state");

    const NodeId realFile = realTree.addNode(realTree.rootId(), L"file.txt", L"C:\\Users\\Example\\file.txt",
                                             FileKind::File, L"", L"0 bytes", L"Text");
    realTree.setChildrenLoaded(realFile, true);
    require(!realTree.node(realFile).childrenLoaded, "setChildrenLoaded should ignore file nodes");

    bool rejectedReplaceOnFile = false;
    try {
        realTree.replaceChildren(realFile, {});
    } catch (const std::invalid_argument&) {
        rejectedReplaceOnFile = true;
    }
    require(rejectedReplaceOnFile, "replaceChildren should reject a non-folder parent");

    std::cout << "FileTreeTests passed\n";
    return 0;
}
