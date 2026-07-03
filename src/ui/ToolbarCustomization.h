#pragma once

#include "settings/AppSettings.h"

#include <string>
#include <vector>

namespace finderx::ui {

std::wstring toolbarCommandLabel(ToolbarCommand command);
std::vector<ToolbarCommand> availableToolbarCommands();
bool addToolbarCommand(std::vector<ToolbarCommand>& commands, ToolbarCommand command);
bool removeToolbarCommand(std::vector<ToolbarCommand>& commands, std::size_t index);
bool moveToolbarCommand(std::vector<ToolbarCommand>& commands, std::size_t index, int direction);

} // namespace finderx::ui
