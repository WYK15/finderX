# FinderX Basic File Actions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add refresh, right-click context menu, copy path, reveal in Explorer, and open actions to FinderX.

**Architecture:** Keep Win32 shell operations in a small `ShellActions` module. Let `FinderListView` expose row targeting helpers, and keep `MainWindow` responsible for menus, commands, refresh, and error status.

**Tech Stack:** C++20 conservative subset, CMake, MSVC, Win32, ShellExecuteW, Windows clipboard APIs.

---

## File Structure

Create:

- `src/shell/ShellActions.h`: shell action API for opening, revealing, and copying paths.
- `src/shell/ShellActions.cpp`: Win32 implementations.

Modify:

- `CMakeLists.txt`: add `src/shell/ShellActions.cpp` to `FinderX` app sources.
- `src/ui/FinderListView.h`: add row hit/selection helpers.
- `src/ui/FinderListView.cpp`: implement row hit/selection helpers.
- `src/ui/MainWindow.h`: add context menu state and file action helper methods.
- `src/ui/MainWindow.cpp`: wire right-click menu, command dispatch, refresh, and keyboard shortcuts.

---

### Task 1: ShellActions Module

**Files:**
- Create: `src/shell/ShellActions.h`
- Create: `src/shell/ShellActions.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add ShellActions header**

Create `src/shell/ShellActions.h`:

```cpp
#pragma once

#include <windows.h>

#include <string>

namespace finderx::shell {

bool openPath(HWND owner, const std::wstring& path);
bool revealInExplorer(HWND owner, const std::wstring& path);
bool copyPathToClipboard(HWND owner, const std::wstring& path);

}
```

- [ ] **Step 2: Add ShellActions implementation**

Create `src/shell/ShellActions.cpp`:

```cpp
#include "shell/ShellActions.h"

#include <shellapi.h>

#include <cstring>

namespace finderx::shell {
namespace {

bool shellExecuteOk(HINSTANCE result) {
    return reinterpret_cast<INT_PTR>(result) > 32;
}

std::wstring explorerSelectArgument(const std::wstring& path) {
    return L"/select,\"" + path + L"\"";
}

} // namespace

bool openPath(HWND owner, const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    return shellExecuteOk(ShellExecuteW(owner, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
}

bool revealInExplorer(HWND owner, const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    const std::wstring arguments = explorerSelectArgument(path);
    return shellExecuteOk(ShellExecuteW(owner, L"open", L"explorer.exe", arguments.c_str(), nullptr, SW_SHOWNORMAL));
}

bool copyPathToClipboard(HWND owner, const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    if (!OpenClipboard(owner)) {
        return false;
    }

    const auto closeClipboard = []() {
        CloseClipboard();
    };

    if (!EmptyClipboard()) {
        closeClipboard();
        return false;
    }

    const SIZE_T bytes = (path.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        closeClipboard();
        return false;
    }

    void* locked = GlobalLock(memory);
    if (!locked) {
        GlobalFree(memory);
        closeClipboard();
        return false;
    }

    std::memcpy(locked, path.c_str(), bytes);
    GlobalUnlock(memory);

    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        closeClipboard();
        return false;
    }

    closeClipboard();
    return true;
}

}
```

- [ ] **Step 3: Update CMake app sources**

Modify `CMakeLists.txt`:

```cmake
set(FINDERX_APP_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/shell/ShellActions.cpp
    src/ui/FinderChrome.cpp
    src/ui/FinderListView.cpp
    src/ui/MainWindow.cpp
    src/ui/RenderContext.cpp
)
```

- [ ] **Step 4: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: Debug app builds.

- [ ] **Step 5: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/shell
git -c safe.directory=D:/finderX commit -m "feat: add shell file actions"
```

---

### Task 2: List Targeting Helpers

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`

- [ ] **Step 1: Add public helper declarations**

Modify `src/ui/FinderListView.h`:

```cpp
    NodeId nodeAtPoint(float x, float y, const D2D1_RECT_F& bounds);
    bool selectNode(NodeId id);
```

Place them near `selectedNode()`.

- [ ] **Step 2: Implement `nodeAtPoint`**

Add to `src/ui/FinderListView.cpp` after `selectedNode()`:

```cpp
NodeId FinderListView::nodeAtPoint(float x, float y, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return kInvalidNodeId;
    }

    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const int index = hitTestRow(x, y, bounds);
    if (index < 0) {
        return kInvalidNodeId;
    }

    return rows_[static_cast<std::size_t>(index)].nodeId;
}
```

- [ ] **Step 3: Implement `selectNode`**

Add:

```cpp
bool FinderListView::selectNode(NodeId id) {
    if (!tree_ || id == kInvalidNodeId) {
        return false;
    }

    rebuildRows();
    const auto row = std::find_if(rows_.begin(), rows_.end(), [id](const VisibleRow& visible) {
        return visible.nodeId == id;
    });
    if (row == rows_.end()) {
        return false;
    }

    if (selected_ == id) {
        return false;
    }

    selected_ = id;
    ensureSelectionVisible();
    return true;
}
```

- [ ] **Step 4: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: Debug app builds.

- [ ] **Step 5: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.h src/ui/FinderListView.cpp
git -c safe.directory=D:/finderX commit -m "feat: expose list row targeting"
```

---

### Task 3: Context Menu And Refresh Integration

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add state and helper declarations**

Modify `src/ui/MainWindow.h`.

Add private methods:

```cpp
    void showContextMenu(POINT clientPoint, POINT screenPoint);
    void handleCommand(WPARAM wParam);
    void openContextNode();
    void revealContextNode();
    void copyContextNodePath();
    bool refreshCurrentDirectory();
    std::wstring selectedNodePath() const;
    bool selectNodeByPath(const std::wstring& path);
```

Add member:

```cpp
    NodeId contextNode_ = kInvalidNodeId;
```

- [ ] **Step 2: Add command ids**

In the anonymous namespace of `src/ui/MainWindow.cpp`, add:

```cpp
constexpr UINT kCommandOpen = 1001;
constexpr UINT kCommandReveal = 1002;
constexpr UINT kCommandCopyPath = 1003;
constexpr UINT kCommandRefresh = 1004;
```

Add include:

```cpp
#include "shell/ShellActions.h"
```

- [ ] **Step 3: Wire right-click message**

Add to `MainWindow::handleMessage`:

```cpp
    case WM_RBUTTONDOWN: {
        SetFocus(hwnd_);
        POINT clientPoint{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        POINT screenPoint = clientPoint;
        ClientToScreen(hwnd_, &screenPoint);
        showContextMenu(clientPoint, screenPoint);
        return 0;
    }
```

- [ ] **Step 4: Wire WM_COMMAND**

Add:

```cpp
    case WM_COMMAND:
        handleCommand(wParam);
        return 0;
```

- [ ] **Step 5: Wire refresh keyboard shortcuts**

At the start of `WM_KEYDOWN`, before Backspace:

```cpp
        if (wParam == VK_F5 || (wParam == L'R' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            refreshCurrentDirectory();
            return 0;
        }
```

- [ ] **Step 6: Implement context menu**

Add:

```cpp
void MainWindow::showContextMenu(POINT clientPoint, POINT screenPoint) {
    const LayoutRects rects = currentLayout();
    contextNode_ = listView_.nodeAtPoint(static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y), rects.list);

    const bool hasTarget = contextNode_ != kInvalidNodeId && contextNode_ < tree_.nodes().size();
    if (hasTarget && listView_.selectNode(contextNode_)) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    if (hasTarget) {
        AppendMenuW(menu, MF_STRING, kCommandOpen, L"Open");
        AppendMenuW(menu, MF_STRING, kCommandReveal, L"Show in Explorer");
        AppendMenuW(menu, MF_STRING, kCommandCopyPath, L"Copy Path");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(menu, MF_STRING, kCommandRefresh, L"Refresh");

    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}
```

- [ ] **Step 7: Implement command dispatch**

Add:

```cpp
void MainWindow::handleCommand(WPARAM wParam) {
    switch (LOWORD(wParam)) {
    case kCommandOpen:
        openContextNode();
        break;
    case kCommandReveal:
        revealContextNode();
        break;
    case kCommandCopyPath:
        copyContextNodePath();
        break;
    case kCommandRefresh:
        refreshCurrentDirectory();
        break;
    default:
        break;
    }
}
```

- [ ] **Step 8: Implement target helpers**

Add:

```cpp
std::wstring MainWindow::selectedNodePath() const {
    const NodeId selected = listView_.selectedNode();
    if (selected == kInvalidNodeId || selected >= tree_.nodes().size()) {
        return L"";
    }
    return tree_.node(selected).path;
}

bool MainWindow::selectNodeByPath(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    for (const FileNode& node : tree_.nodes()) {
        if (node.id != kInvalidNodeId && node.path == path) {
            return listView_.selectNode(node.id);
        }
    }
    return false;
}
```

- [ ] **Step 9: Implement context actions**

Add:

```cpp
void MainWindow::openContextNode() {
    if (contextNode_ == kInvalidNodeId || contextNode_ >= tree_.nodes().size()) {
        return;
    }
    activateNode(contextNode_);
}

void MainWindow::revealContextNode() {
    if (contextNode_ == kInvalidNodeId || contextNode_ >= tree_.nodes().size()) {
        return;
    }
    if (!shell::revealInExplorer(hwnd_, tree_.node(contextNode_).path)) {
        setStatusText(L"Cannot show in Explorer");
    }
}

void MainWindow::copyContextNodePath() {
    if (contextNode_ == kInvalidNodeId || contextNode_ >= tree_.nodes().size()) {
        return;
    }
    if (!shell::copyPathToClipboard(hwnd_, tree_.node(contextNode_).path)) {
        setStatusText(L"Cannot copy path");
    }
}
```

- [ ] **Step 10: Use ShellActions for file open**

Modify `openFile`:

```cpp
void MainWindow::openFile(const std::wstring& path) {
    if (!shell::openPath(hwnd_, path)) {
        setStatusText(L"Cannot open file");
    }
}
```

Remove `#include <shellapi.h>` from `MainWindow.cpp` if no longer needed.

- [ ] **Step 11: Implement refresh**

Add:

```cpp
bool MainWindow::refreshCurrentDirectory() {
    const std::wstring currentPath = history_.currentPath();
    if (currentPath.empty()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    const std::wstring previousSelection = selectedNodePath();
    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(currentPath);
    if (!result.ok()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

    std::wstring rootName = fileNameFromPath(currentPath);
    if (rootName.empty()) {
        rootName = currentPath;
    }

    tree_ = FileTree(currentPath, std::move(rootName));
    tree_.replaceChildren(tree_.rootId(), std::move(result.children));
    listView_ = FinderListView(&tree_);
    selectNodeByPath(previousSelection);

    chromeState_.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}
```

- [ ] **Step 12: Build app and tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_model_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_fs_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_navigation_tests
.\build\Debug\finderx_model_tests.exe
.\build\Debug\finderx_fs_tests.exe
.\build\Debug\finderx_navigation_tests.exe
```

Expected:

```text
FileTreeTests passed
DirectoryLoaderTests passed
NavigationHistoryTests passed
```

- [ ] **Step 13: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add context menu file actions"
```

---

### Task 4: Verification And Manual Smoke Check

**Files:**
- Modify only if verification exposes a defect.

- [ ] **Step 1: Configure**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
```

Expected: configure and generate succeed.

- [ ] **Step 2: Build Debug app and tests**

Run sequentially:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_model_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_fs_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_navigation_tests
```

Expected: all targets build.

- [ ] **Step 3: Run tests**

Run:

```powershell
.\build\Debug\finderx_model_tests.exe
.\build\Debug\finderx_fs_tests.exe
.\build\Debug\finderx_navigation_tests.exe
```

Expected:

```text
FileTreeTests passed
DirectoryLoaderTests passed
NavigationHistoryTests passed
```

- [ ] **Step 4: Build Release app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target FinderX
```

Expected:

- `D:\finderX\build\Debug\FinderX.exe`
- `D:\finderX\build\Release\FinderX.exe`

- [ ] **Step 5: Short app launch**

Run:

```powershell
$process = Start-Process -FilePath .\build\Debug\FinderX.exe -PassThru
Start-Sleep -Seconds 3
$alive = -not $process.HasExited
if ($alive) {
    Stop-Process -Id $process.Id -Force
    'FinderX started and remained running for 3 seconds'
} else {
    "FinderX exited early with code $($process.ExitCode)"
    exit 1
}
```

Expected:

```text
FinderX started and remained running for 3 seconds
```

- [ ] **Step 6: Manual action checklist**

Open `D:\finderX\build\Debug\FinderX.exe` and check:

- Right-click a row selects it and shows Open, Show in Explorer, Copy Path, Refresh.
- Right-click blank list area shows only Refresh.
- Open on a folder enters the folder.
- Open on a file launches the default Windows app.
- Show in Explorer opens Explorer for the item.
- Copy Path places the item path on the clipboard.
- F5 refreshes current directory.
- Ctrl+R refreshes current directory.
- Refresh does not change Back/Forward history.
- Existing navigation, lazy expansion, and visual styling still work.

- [ ] **Step 7: Static checks**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected: no whitespace errors; only pre-existing intentional untracked files may remain.

- [ ] **Step 8: Commit verification fixes when verification exposes defects**

When fixes were made:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src
git -c safe.directory=D:/finderX commit -m "fix: stabilize basic file actions"
```

When no fixes were made, do not create an empty commit.
