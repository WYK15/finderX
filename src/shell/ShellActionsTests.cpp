#include "shell/ShellActions.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <windows.h>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

std::filesystem::path tempDirectory() {
    wchar_t tempPath[MAX_PATH]{};
    const DWORD length = GetTempPathW(static_cast<DWORD>(std::size(tempPath)), tempPath);
    require(length > 0 && length < std::size(tempPath), "temp path should be available");
    const std::filesystem::path path = std::filesystem::path(tempPath) / (L"finderx-shell-actions-" + std::to_wstring(GetTickCount64()));
    std::filesystem::create_directories(path);
    return path;
}

void testCreateShortcutBesidePath() {
    const std::filesystem::path root = tempDirectory();
    const std::filesystem::path target = root / L"report.txt";
    std::ofstream(target) << "hello";

    std::wstring shortcutPath;
    require(shell::createShortcutBesidePath(nullptr, target.wstring(), shortcutPath), "createShortcutBesidePath should create shortcut");
    require(shortcutPath == (root / L"report - Shortcut.lnk").wstring(), "first shortcut should use base shortcut name");
    require(std::filesystem::exists(shortcutPath), "shortcut file should exist");

    std::wstring secondShortcutPath;
    require(shell::createShortcutBesidePath(nullptr, target.wstring(), secondShortcutPath), "createShortcutBesidePath should create second shortcut");
    require(secondShortcutPath == (root / L"report - Shortcut 2.lnk").wstring(), "second shortcut should avoid existing shortcut name");
    require(std::filesystem::exists(secondShortcutPath), "second shortcut file should exist");

    std::filesystem::remove_all(root);
}

} // namespace

int main() {
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    require(SUCCEEDED(comResult) || comResult == RPC_E_CHANGED_MODE, "COM should initialize for shell action tests");

    testCreateShortcutBesidePath();

    if (SUCCEEDED(comResult)) {
        CoUninitialize();
    }

    std::cout << "ShellActionsTests passed\n";
    return 0;
}
