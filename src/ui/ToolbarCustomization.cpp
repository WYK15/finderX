#include "ui/ToolbarCustomization.h"

#include <algorithm>

namespace finderx::ui {

std::wstring toolbarCommandLabel(ToolbarCommand command) {
    switch (command) {
    case ToolbarCommand::NewFolder:
        return L"New Folder";
    case ToolbarCommand::NewFile:
        return L"New File";
    case ToolbarCommand::Sort:
        return L"Sort";
    case ToolbarCommand::Settings:
        return L"Settings";
    case ToolbarCommand::Search:
    default:
        return L"Search";
    }
}

std::vector<ToolbarCommand> availableToolbarCommands() {
    return {
        ToolbarCommand::NewFolder,
        ToolbarCommand::NewFile,
        ToolbarCommand::Sort,
        ToolbarCommand::Settings,
        ToolbarCommand::Search,
    };
}

bool addToolbarCommand(std::vector<ToolbarCommand>& commands, ToolbarCommand command) {
    if (std::find(commands.begin(), commands.end(), command) != commands.end()) {
        return false;
    }
    commands.push_back(command);
    return true;
}

bool removeToolbarCommand(std::vector<ToolbarCommand>& commands, std::size_t index) {
    if (index >= commands.size()) {
        return false;
    }
    commands.erase(commands.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool moveToolbarCommand(std::vector<ToolbarCommand>& commands, std::size_t index, int direction) {
    if (index >= commands.size() || (direction != -1 && direction != 1)) {
        return false;
    }
    if (direction < 0 && index == 0) {
        return false;
    }
    const std::size_t next = direction < 0 ? index - 1 : index + 1;
    if (next >= commands.size()) {
        return false;
    }
    std::swap(commands[index], commands[next]);
    return true;
}

} // namespace finderx::ui
