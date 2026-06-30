#include "ui/FileOperationState.h"

#include <cstdlib>
#include <iostream>

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

    std::cout << "FileOperationStateTests passed\n";
    return 0;
}
