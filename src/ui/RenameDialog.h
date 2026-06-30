#pragma once

#include <windows.h>

#include <string>

namespace finderx::ui {

bool isValidRenameName(const std::wstring& name);
bool promptForRename(HWND owner, const std::wstring& currentName, std::wstring& newName);

} // namespace finderx::ui
