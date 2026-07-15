#include "ui/FileUndoManager.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    {
        ui::FileUndoManager manager;
        require(!manager.hasUndo(), "new undo manager should start empty");
        manager.recordCreatedPath(L"C:\\Root\\New Text Document.txt");
        require(manager.hasUndo(), "created path should create undo state");
        require(manager.lastOperation().kind == ui::UndoFileOperationKind::CreatedPaths,
                "created path should record created operation");
        require(manager.lastOperation().destinationPaths.size() == 1, "created path should record one destination");
        manager.clear();
        require(!manager.hasUndo(), "clear should remove undo state");
    }

    {
        ui::FileUndoManager manager;
        manager.recordRename(L"C:\\Root\\old.txt", L"C:\\Root\\new.txt");
        require(manager.lastOperation().kind == ui::UndoFileOperationKind::RenamedPath,
                "rename should record rename operation");
        require(manager.lastOperation().sourcePaths.front() == L"C:\\Root\\old.txt", "rename should keep old path");
        require(manager.lastOperation().destinationPaths.front() == L"C:\\Root\\new.txt", "rename should keep new path");
    }

    {
        const std::wstring sources[] = {L"C:\\From\\a.txt", L"C:\\From\\Folder"};
        const std::vector<std::wstring> destinations = ui::pathsInDirectory(sources, L"D:\\To");
        require(destinations.size() == 2, "pathsInDirectory should preserve item count");
        require(destinations[0] == L"D:\\To\\a.txt", "pathsInDirectory should map file name into destination");
        require(destinations[1] == L"D:\\To\\Folder", "pathsInDirectory should map folder name into destination");
    }

    std::cout << "FileUndoManagerTests passed\n";
    return 0;
}
