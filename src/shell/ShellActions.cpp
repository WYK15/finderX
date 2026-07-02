#include "shell/ShellActions.h"

#include <filesystem>
#include <shellapi.h>
#include <shobjidl.h>

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

bool pathExists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring shortcutDisplayStem(const std::filesystem::path& path) {
    std::wstring stem = path.stem().wstring();
    if (stem.empty()) {
        stem = path.filename().wstring();
    }
    if (stem.empty()) {
        stem = L"Shortcut";
    }
    return stem;
}

std::wstring nextShortcutPath(const std::wstring& targetPath) {
    const std::filesystem::path target(targetPath);
    const std::filesystem::path parent = target.parent_path();
    if (parent.empty()) {
        return L"";
    }

    const std::wstring stem = shortcutDisplayStem(target);
    for (int index = 1; index <= 9999; ++index) {
        std::wstring name = stem + L" - Shortcut";
        if (index > 1) {
            name += L" " + std::to_wstring(index);
        }
        name += L".lnk";

        const std::wstring candidate = (parent / name).wstring();
        if (!pathExists(candidate)) {
            return candidate;
        }
    }

    return L"";
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

bool createShortcutBesidePath(HWND, const std::wstring& targetPath, std::wstring& shortcutPath) {
    shortcutPath.clear();
    if (targetPath.empty() || !pathExists(targetPath)) {
        return false;
    }

    const std::wstring outputPath = nextShortcutPath(targetPath);
    if (outputPath.empty()) {
        return false;
    }

    IShellLinkW* link = nullptr;
    const HRESULT createResult = CoCreateInstance(
        CLSID_ShellLink,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&link));
    if (FAILED(createResult) || link == nullptr) {
        return false;
    }

    bool success = false;
    if (SUCCEEDED(link->SetPath(targetPath.c_str()))) {
        const std::filesystem::path parent = std::filesystem::path(targetPath).parent_path();
        if (!parent.empty()) {
            link->SetWorkingDirectory(parent.wstring().c_str());
        }

        IPersistFile* file = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file))) && file != nullptr) {
            if (SUCCEEDED(file->Save(outputPath.c_str(), TRUE))) {
                shortcutPath = outputPath;
                success = true;
            }
            file->Release();
        }
    }

    link->Release();
    return success;
}

}
