#include "shell/ShellActions.h"

#include <filesystem>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <cstring>
#include <new>
#include <utility>
#include <vector>

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

HGLOBAL createFileDropMemory(const std::vector<std::wstring>& paths) {
    if (paths.empty()) {
        return nullptr;
    }

    SIZE_T characterCount = 1;
    for (const std::wstring& path : paths) {
        if (path.empty()) {
            return nullptr;
        }
        characterCount += path.size() + 1;
    }

    const SIZE_T byteCount = sizeof(DROPFILES) + characterCount * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, byteCount);
    if (!memory) {
        return nullptr;
    }

    auto* dropFiles = static_cast<DROPFILES*>(GlobalLock(memory));
    if (!dropFiles) {
        GlobalFree(memory);
        return nullptr;
    }

    dropFiles->pFiles = sizeof(DROPFILES);
    dropFiles->fWide = TRUE;

    auto* cursor = reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(dropFiles) + sizeof(DROPFILES));
    for (const std::wstring& path : paths) {
        std::memcpy(cursor, path.c_str(), path.size() * sizeof(wchar_t));
        cursor += path.size();
        *cursor++ = L'\0';
    }
    *cursor = L'\0';

    GlobalUnlock(memory);
    return memory;
}

class FileDropDataObject final : public IDataObject {
public:
    explicit FileDropDataObject(std::vector<std::wstring> paths)
        : refCount_(1),
          paths_(std::move(paths)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (iid == IID_IUnknown || iid == IID_IDataObject) {
            *object = static_cast<IDataObject*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&refCount_);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG count = InterlockedDecrement(&refCount_);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE GetData(FORMATETC* format, STGMEDIUM* medium) override {
        if (!format || !medium) {
            return E_INVALIDARG;
        }
        if (!supportsFileDrop(*format)) {
            return DV_E_FORMATETC;
        }

        HGLOBAL memory = createFileDropMemory(paths_);
        if (!memory) {
            return E_OUTOFMEMORY;
        }

        medium->tymed = TYMED_HGLOBAL;
        medium->hGlobal = memory;
        medium->pUnkForRelease = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*, STGMEDIUM*) override {
        return DATA_E_FORMATETC;
    }

    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* format) override {
        if (!format) {
            return E_INVALIDARG;
        }
        return supportsFileDrop(*format) ? S_OK : DV_E_FORMATETC;
    }

    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*, FORMATETC* formatOut) override {
        if (formatOut) {
            formatOut->ptd = nullptr;
        }
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetData(FORMATETC*, STGMEDIUM*, BOOL) override {
        return DATA_E_FORMATETC;
    }

    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD direction, IEnumFORMATETC** enumFormatEtc) override {
        if (!enumFormatEtc) {
            return E_POINTER;
        }
        *enumFormatEtc = nullptr;
        if (direction != DATADIR_GET) {
            return E_NOTIMPL;
        }

        FORMATETC format{};
        format.cfFormat = CF_HDROP;
        format.dwAspect = DVASPECT_CONTENT;
        format.lindex = -1;
        format.tymed = TYMED_HGLOBAL;
        return SHCreateStdEnumFmtEtc(1, &format, enumFormatEtc);
    }

    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override {
        return OLE_E_ADVISENOTSUPPORTED;
    }

    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override {
        return OLE_E_ADVISENOTSUPPORTED;
    }

private:
    static bool supportsFileDrop(const FORMATETC& format) {
        return format.cfFormat == CF_HDROP
            && (format.tymed & TYMED_HGLOBAL) != 0
            && format.dwAspect == DVASPECT_CONTENT;
    }

    LONG refCount_;
    std::vector<std::wstring> paths_;
};

class FileDropSource final : public IDropSource {
public:
    FileDropSource() : refCount_(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override {
        if (!object) {
            return E_POINTER;
        }
        *object = nullptr;
        if (iid == IID_IUnknown || iid == IID_IDropSource) {
            *object = static_cast<IDropSource*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&refCount_);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG count = InterlockedDecrement(&refCount_);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL escapePressed, DWORD keyState) override {
        if (escapePressed) {
            return DRAGDROP_S_CANCEL;
        }
        if ((keyState & MK_LBUTTON) == 0) {
            return DRAGDROP_S_DROP;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD) override {
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

private:
    LONG refCount_;
};

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

FileDragResult beginFileDragDrop(const std::vector<std::wstring>& paths) {
    if (paths.empty()) {
        return FileDragResult::None;
    }

    auto* dataObject = new (std::nothrow) FileDropDataObject(paths);
    auto* dropSource = new (std::nothrow) FileDropSource();
    if (!dataObject || !dropSource) {
        if (dataObject) {
            dataObject->Release();
        }
        if (dropSource) {
            dropSource->Release();
        }
        return FileDragResult::None;
    }

    DWORD effect = DROPEFFECT_NONE;
    const HRESULT result = DoDragDrop(
        dataObject,
        dropSource,
        DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK,
        &effect);

    dataObject->Release();
    dropSource->Release();

    if (result == DRAGDROP_S_CANCEL) {
        return FileDragResult::Cancelled;
    }
    if (FAILED(result) || effect == DROPEFFECT_NONE) {
        return FileDragResult::None;
    }
    if ((effect & DROPEFFECT_MOVE) != 0) {
        return FileDragResult::Moved;
    }
    if ((effect & DROPEFFECT_LINK) != 0) {
        return FileDragResult::Linked;
    }
    if ((effect & DROPEFFECT_COPY) != 0) {
        return FileDragResult::Copied;
    }
    return FileDragResult::None;
}

}
