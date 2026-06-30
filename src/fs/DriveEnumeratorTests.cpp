#include "fs/DriveEnumerator.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void testDriveRootsFromMask() {
    const std::vector<std::wstring> roots = driveRootsFromMaskForTests((1u << 0) | (1u << 2) | (1u << 25));

    require(roots.size() == 3, "drive roots should include three entries");
    require(roots[0] == L"A:\\", "first drive root should be A");
    require(roots[1] == L"C:\\", "second drive root should be C");
    require(roots[2] == L"Z:\\", "third drive root should be Z");
}

static void testDriveNodesFromRoots() {
    const std::vector<FileNode> nodes = driveNodesFromRootsForTests({L"C:\\", L"D:\\"});

    require(nodes.size() == 2, "drive nodes should include two rows");
    require(nodes[0].name == L"C:\\", "first drive name should equal root");
    require(nodes[0].path == L"C:\\", "first drive path should equal root");
    require(nodes[0].kind == FileKind::Folder, "first drive kind should be folder");
    require(nodes[0].modifiedTicks == 0, "first drive modified ticks should be zero");
    require(nodes[0].sizeBytes == 0, "first drive size bytes should be zero");
    require(!nodes[0].childrenLoaded, "first drive children should not be loaded");
    require(nodes[1].name == L"D:\\", "second drive name should equal root");
    require(nodes[1].path == L"D:\\", "second drive path should equal root");
    require(nodes[1].kind == FileKind::Folder, "second drive kind should be folder");
    require(nodes[1].modifiedTicks == 0, "second drive modified ticks should be zero");
    require(nodes[1].sizeBytes == 0, "second drive size bytes should be zero");
    require(!nodes[1].childrenLoaded, "second drive children should not be loaded");
}

int main() {
    try {
        testDriveRootsFromMask();
        testDriveNodesFromRoots();
        std::cout << "DriveEnumeratorTests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
}
