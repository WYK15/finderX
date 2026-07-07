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

    require(toolbarCommandLabel(ToolbarCommand::NewFolder) == L"新建文件夹", "new folder should have a localized label");
    require(toolbarCommandLabel(ToolbarCommand::NewFile) == L"新建文件", "new file should have a localized label");
    require(toolbarCommandLabel(ToolbarCommand::PowerShell) == L"在此处打开 PowerShell", "PowerShell should have a localized toolbar label");
    require(availableToolbarCommands().size() == 6, "customization should expose six toolbar commands");

    std::vector<ToolbarCommand> commands = {ToolbarCommand::Search};
    require(addToolbarCommand(commands, ToolbarCommand::PowerShell), "adding PowerShell command should succeed");
    require(commands[1] == ToolbarCommand::PowerShell, "PowerShell command should append");
    require(addToolbarCommand(commands, ToolbarCommand::NewFolder), "adding missing command should succeed");
    require(commands[2] == ToolbarCommand::NewFolder, "new command should append");
    require(!addToolbarCommand(commands, ToolbarCommand::NewFolder), "adding duplicate command should fail");

    require(moveToolbarCommand(commands, 1, -1), "moving command up should succeed");
    require(commands[0] == ToolbarCommand::PowerShell, "moved command should occupy previous slot");
    require(removeToolbarCommand(commands, 0), "removing command should succeed");
    require(commands[0] == ToolbarCommand::Search, "remaining command should shift into first slot");

    std::cout << "ToolbarCustomizationTests passed\n";
    return 0;
}
