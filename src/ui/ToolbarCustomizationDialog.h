#pragma once

#include "settings/AppSettings.h"

#include <windows.h>

namespace finderx::ui {

bool promptForToolbarCustomization(HWND owner, AppSettings& settings);

} // namespace finderx::ui
