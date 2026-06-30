#include "fs/FileSorter.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

FileNode node(std::wstring name,
              FileKind kind,
              unsigned long long modifiedTicks,
              unsigned long long sizeBytes,
              std::wstring kindText = L"File") {
    FileNode result;
    result.name = std::move(name);
    result.kind = kind;
    result.modifiedTicks = modifiedTicks;
    result.sizeBytes = sizeBytes;
    result.kindText = std::move(kindText);
    return result;
}

void testNameAscendingFoldersFirst() {
    std::vector<FileNode> nodes{
        node(L"zeta.txt", FileKind::File, 0, 10),
        node(L"beta", FileKind::Folder, 0, 0, L"Folder"),
        node(L"Alpha.txt", FileKind::File, 0, 20),
        node(L"alpha", FileKind::Folder, 0, 0, L"Folder"),
    };

    sortFileNodes(nodes, SortColumn::Name, SortDirection::Ascending);

    require(nodes[0].name == L"alpha", "name ascending should put alpha folder first");
    require(nodes[1].name == L"beta", "name ascending should put beta folder second");
    require(nodes[2].name == L"Alpha.txt", "name ascending should put Alpha file first");
    require(nodes[3].name == L"zeta.txt", "name ascending should put zeta file last");
}

void testNameDescendingFoldersFirst() {
    std::vector<FileNode> nodes{
        node(L"Alpha.txt", FileKind::File, 0, 10),
        node(L"alpha", FileKind::Folder, 0, 0, L"Folder"),
        node(L"zeta.txt", FileKind::File, 0, 20),
        node(L"beta", FileKind::Folder, 0, 0, L"Folder"),
    };

    sortFileNodes(nodes, SortColumn::Name, SortDirection::Descending);

    require(nodes[0].kind == FileKind::Folder, "descending should still keep folders first");
    require(nodes[1].kind == FileKind::Folder, "descending should keep both folders first");
    require(nodes[0].name == L"beta", "name descending should sort folders by selected column");
    require(nodes[1].name == L"alpha", "name descending should sort folders descending");
    require(nodes[2].name == L"zeta.txt", "name descending should sort files descending");
    require(nodes[3].name == L"Alpha.txt", "name descending should sort files descending");
}

void testModifiedAscendingAndDescending() {
    std::vector<FileNode> nodes{
        node(L"newer.txt", FileKind::File, 30, 10),
        node(L"older.txt", FileKind::File, 10, 10),
        node(L"middle.txt", FileKind::File, 20, 10),
    };

    sortFileNodes(nodes, SortColumn::Modified, SortDirection::Ascending);
    require(nodes[0].name == L"older.txt", "modified ascending should put oldest first");
    require(nodes[2].name == L"newer.txt", "modified ascending should put newest last");

    sortFileNodes(nodes, SortColumn::Modified, SortDirection::Descending);
    require(nodes[0].name == L"newer.txt", "modified descending should put newest first");
    require(nodes[2].name == L"older.txt", "modified descending should put oldest last");
}

void testSizeAscending() {
    std::vector<FileNode> nodes{
        node(L"large.txt", FileKind::File, 0, 300),
        node(L"small.txt", FileKind::File, 0, 10),
        node(L"medium.txt", FileKind::File, 0, 200),
    };

    sortFileNodes(nodes, SortColumn::Size, SortDirection::Ascending);

    require(nodes[0].name == L"small.txt", "size ascending should put smallest first");
    require(nodes[1].name == L"medium.txt", "size ascending should put medium second");
    require(nodes[2].name == L"large.txt", "size ascending should put largest last");
}

void testKindAscendingAndNameTieBreak() {
    std::vector<FileNode> nodes{
        node(L"Beta.log", FileKind::File, 0, 10, L"Text Document"),
        node(L"alpha.bin", FileKind::File, 0, 10, L"Binary File"),
        node(L"alpha.log", FileKind::File, 0, 10, L"Text Document"),
    };

    sortFileNodes(nodes, SortColumn::Kind, SortDirection::Ascending);

    require(nodes[0].name == L"alpha.bin", "kind ascending should sort by kind text");
    require(nodes[1].name == L"alpha.log", "kind ties should fall back to name ascending");
    require(nodes[2].name == L"Beta.log", "kind ties should fall back to case-insensitive name ascending");
}

} // namespace

int main() {
    testNameAscendingFoldersFirst();
    testNameDescendingFoldersFirst();
    testModifiedAscendingAndDescending();
    testSizeAscending();
    testKindAscendingAndNameTieBreak();
    std::cout << "FileSorterTests passed\n";
}
