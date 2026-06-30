#include "ui/FileOperationState.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using finderx::ui::FileOperationState;
    using finderx::ui::PendingFileOperationKind;

    {
        FileOperationState state;
        require(!state.hasPendingOperation(), "new state should not have pending operation");
        require(state.kind() == PendingFileOperationKind::None, "new state kind should be none");
        require(state.sourcePath().empty(), "new state source path should be empty");
    }

    {
        FileOperationState state;
        state.setCopy(L"C:\\Source\\Report.txt");
        state.markPasteSucceeded();
        require(state.hasPendingOperation(), "copy should remain pending after paste");
        require(state.kind() == PendingFileOperationKind::Copy, "copy kind should persist after paste");
        require(state.sourcePath() == L"C:\\Source\\Report.txt", "copy path should persist after paste");
    }

    {
        FileOperationState state;
        state.setCut(L"C:\\Source\\Report.txt");
        state.markPasteSucceeded();
        require(!state.hasPendingOperation(), "cut should clear after paste");
        require(state.kind() == PendingFileOperationKind::None, "cut paste should clear kind");
        require(state.sourcePath().empty(), "cut paste should clear path");
    }

    {
        FileOperationState state;
        state.setCopy(L"C:\\Source\\Report.txt");
        state.setCopy(L"");
        require(!state.hasPendingOperation(), "empty copy should clear state");

        state.setCut(L"C:\\Source\\Report.txt");
        state.setCut(L"");
        require(!state.hasPendingOperation(), "empty cut should clear state");
    }

    {
        FileOperationState state;
        state.setCopy(std::vector<std::wstring>{L"C:\\Temp\\a.txt", L"C:\\Temp\\b.txt"});
        require(state.hasPendingOperation(), "multi copy should set pending operation");
        require(state.sourcePaths().size() == 2, "multi copy should store both paths");
        state.markPasteSucceeded();
        require(state.sourcePaths().size() == 2, "multi copy should remain after paste succeeds");
    }

    {
        FileOperationState state;
        state.setCut(std::vector<std::wstring>{L"C:\\Temp\\c.txt", L"", L"C:\\Temp\\d.txt"});
        require(state.kind() == PendingFileOperationKind::Cut, "multi cut kind should be stored");
        require(state.sourcePaths().size() == 2, "empty paths should be ignored");
        state.markPasteSucceeded();
        require(!state.hasPendingOperation(), "multi cut should clear after paste succeeds");
    }

    {
        FileOperationState state;
        state.setCopy(std::vector<std::wstring>{L""});
        require(!state.hasPendingOperation(), "all-empty copy path list should clear state");
    }

    std::cout << "FileOperationStateTests passed\n";
    return 0;
}
