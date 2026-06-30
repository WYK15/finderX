#include "shell/ShellActions.h"

#include <shellapi.h>

#include <cstring>

namespace finderx::shell {
namespace {

bool shellExecuteOk(HINSTANCE result) {
    return reinterpret_cast<INT_PTR>(result) > 32;
}

bool directoryExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

}

bool openPath(HWND owner, const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    const HINSTANCE result = ShellExecuteW(owner, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return shellExecuteOk(result);
}

bool revealInExplorer(HWND owner, const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    const std::wstring args = L"/select,\"" + path + L"\"";
    const HINSTANCE result = ShellExecuteW(owner, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
    return shellExecuteOk(result);
}

bool openPowerShellAt(HWND owner, const std::wstring& directory) {
    if (!directoryExists(directory)) {
        return false;
    }

    const HINSTANCE result = ShellExecuteW(owner, L"open", L"powershell.exe", nullptr, directory.c_str(), SW_SHOWNORMAL);
    return shellExecuteOk(result);
}

bool copyPathToClipboard(HWND owner, const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    if (!OpenClipboard(owner)) {
        return false;
    }

    bool success = false;
    HGLOBAL memory = nullptr;

    if (EmptyClipboard()) {
        const SIZE_T byteCount = (path.size() + 1) * sizeof(wchar_t);
        memory = GlobalAlloc(GMEM_MOVEABLE, byteCount);
        if (memory != nullptr) {
            void* buffer = GlobalLock(memory);
            if (buffer != nullptr) {
                std::memcpy(buffer, path.c_str(), byteCount);
                GlobalUnlock(memory);

                if (SetClipboardData(CF_UNICODETEXT, memory) != nullptr) {
                    memory = nullptr;
                    success = true;
                }
            }
        }
    }

    if (memory != nullptr) {
        GlobalFree(memory);
    }

    CloseClipboard();
    return success;
}

}
