#pragma once

#include <windows.h>

#include <string>

namespace finderx::shell {

bool openPath(HWND owner, const std::wstring& path);
bool revealInExplorer(HWND owner, const std::wstring& path);
bool copyPathToClipboard(HWND owner, const std::wstring& path);
bool openPowerShellAt(HWND owner, const std::wstring& directory);

}
