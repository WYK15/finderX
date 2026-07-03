#include "ui/ToolbarCustomization.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    using finderx::ToolbarCommand;
    using finderx::ui::addToolbarCommand;
    using finderx::ui::availableToolbarCommands;
    using finderx::ui::moveToolbarCommand;
    using finderx::ui::removeToolbarCommand;
    using finderx::ui::toolbarCommandLabel;

    require(toolbarCommandLabel(ToolbarCommand::NewFolder) == L"New Folder", "new folder should have a label");
    require(toolbarCommandLabel(ToolbarCommand::NewFile) == L"New File", "new file should have a label");
    require(availableToolbarCommands().size() == 5, "customization should expose five toolbar commands");

    std::vector<ToolbarCommand> commands = {ToolbarCommand::Search};
    require(addToolbarCommand(commands, ToolbarCommand::NewFolder), "adding missing command should succeed");
    require(commands[1] == ToolbarCommand::NewFolder, "new command should append");
    require(!addToolbarCommand(commands, ToolbarCommand::NewFolder), "adding duplicate command should fail");

    require(moveToolbarCommand(commands, 1, -1), "moving command up should succeed");
    require(commands[0] == ToolbarCommand::NewFolder, "moved command should occupy previous slot");
    require(removeToolbarCommand(commands, 0), "removing command should succeed");
    require(commands[0] == ToolbarCommand::Search, "remaining command should shift into first slot");

    std::cout << "ToolbarCustomizationTests passed\n";
    return 0;
}
