#pragma once

#include <windows.h>

#include <string>

namespace finderx::shell {

bool openPath(HWND owner, const std::wstring& path);
bool revealInExplorer(HWND owner, const std::wstring& path);
bool copyPathToClipboard(HWND owner, const std::wstring& path);
bool openPowerShellAt(HWND owner, const std::wstring& directory);
bool createShortcutBesidePath(HWND owner, const std::wstring& targetPath, std::wstring& shortcutPath);

}
