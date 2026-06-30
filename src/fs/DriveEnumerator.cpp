#include "fs/DriveEnumerator.h"

#include <Windows.h>

namespace finderx {
namespace {

std::vector<std::wstring> driveRootsFromMask(unsigned long mask) {
    std::vector<std::wstring> roots;

    for (int driveIndex = 0; driveIndex < 26; ++driveIndex) {
        if ((mask & (1ul << driveIndex)) == 0) {
            continue;
        }

        std::wstring root = L"A:\\";
        root[0] = static_cast<wchar_t>(L'A' + driveIndex);
        roots.push_back(root);
    }

    return roots;
}

FileNode driveNode(const std::wstring& root) {
    FileNode node;
    node.name = root;
    node.path = root;
    node.modified = L"--";
    node.size = L"--";
    node.kindText = L"Drive";
    node.modifiedTicks = 0;
    node.sizeBytes = 0;
    node.kind = FileKind::Folder;
    node.expanded = false;
    node.childrenLoaded = false;
    return node;
}

} // namespace

std::vector<std::wstring> enumerateDriveRoots() {
    return driveRootsFromMask(GetLogicalDrives());
}

std::vector<FileNode> enumerateDriveNodes() {
    return driveNodesFromRootsForTests(enumerateDriveRoots());
}

std::vector<std::wstring> driveRootsFromMaskForTests(unsigned long mask) {
    return driveRootsFromMask(mask);
}

std::vector<FileNode> driveNodesFromRootsForTests(const std::vector<std::wstring>& roots) {
    std::vector<FileNode> nodes;
    nodes.reserve(roots.size());

    for (const auto& root : roots) {
        nodes.push_back(driveNode(root));
    }

    return nodes;
}

} // namespace finderx
