# FinderX File Operations MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add single-item rename, move to trash, copy, cut, and paste to FinderX.

**Architecture:** Keep filesystem mutations behind focused shell/helper modules. `MainWindow` owns command routing, menu state, refresh behavior, and selected item targeting; helper modules own validation, pending operation state, shell file operations, and the rename modal.

**Tech Stack:** C++20, Win32, CMake, Direct2D UI, Windows Shell APIs, existing simple executable tests.

---

## File Structure

- Create `src/shell/ShellFileOperations.h`: public shell file operation API and `FileOperationResult`.
- Create `src/shell/ShellFileOperations.cpp`: rename, recycle-bin delete, copy, and move wrappers.
- Create `src/ui/FileOperationState.h`: pending copy/cut state object.
- Create `src/ui/FileOperationState.cpp`: state transitions for copy, cut, paste success, and clear.
- Create `src/ui/FileOperationStateTests.cpp`: focused tests for pending operation state.
- Create `src/ui/RenameDialog.h`: modal rename dialog API and filename validation declaration.
- Create `src/ui/RenameDialog.cpp`: native modal rename dialog and filename validation.
- Create `src/ui/RenameDialogTests.cpp`: filename validation tests.
- Modify `CMakeLists.txt`: add helper sources to `FinderX`; add two new test executables.
- Modify `src/ui/MainWindow.h`: add file-operation command methods and pending operation member.
- Modify `src/ui/MainWindow.cpp`: add command ids, context menu items, keyboard shortcuts, and operation handlers.

---

### Task 1: Rename Validation And Pending Operation State

**Files:**
- Create: `src/ui/RenameDialog.h`
- Create: `src/ui/RenameDialog.cpp`
- Create: `src/ui/RenameDialogTests.cpp`
- Create: `src/ui/FileOperationState.h`
- Create: `src/ui/FileOperationState.cpp`
- Create: `src/ui/FileOperationStateTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create rename validation API**

Add `src/ui/RenameDialog.h`:

```cpp
#pragma once

#include <windows.h>

#include <string>

namespace finderx::ui {

bool isValidRenameName(const std::wstring& name);
bool promptForRename(HWND owner, const std::wstring& currentName, std::wstring& newName);

}
```

- [ ] **Step 2: Implement validation with a stub dialog**

Add `src/ui/RenameDialog.cpp`:

```cpp
#include "ui/RenameDialog.h"

namespace finderx::ui {
namespace {

bool hasInvalidCharacter(wchar_t ch) {
    switch (ch) {
    case L'\\':
    case L'/':
    case L':':
    case L'*':
    case L'?':
    case L'"':
    case L'<':
    case L'>':
    case L'|':
        return true;
    default:
        return false;
    }
}

}

bool isValidRenameName(const std::wstring& name) {
    if (name.empty()) {
        return false;
    }

    for (wchar_t ch : name) {
        if (hasInvalidCharacter(ch)) {
            return false;
        }
    }
    return true;
}

bool promptForRename(HWND, const std::wstring&, std::wstring&) {
    return false;
}

}
```

- [ ] **Step 3: Create validation tests**

Add `src/ui/RenameDialogTests.cpp`:

```cpp
#include "ui/RenameDialog.h"

#include <cstdlib>
#include <iostream>

using namespace finderx::ui;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

int main() {
    require(isValidRenameName(L"Report.txt"), "normal filename should be valid");
    require(isValidRenameName(L"项目.txt"), "unicode filename should be valid");
    require(!isValidRenameName(L""), "empty filename should be invalid");
    require(!isValidRenameName(L"a\\b"), "backslash should be invalid");
    require(!isValidRenameName(L"a/b"), "slash should be invalid");
    require(!isValidRenameName(L"a:b"), "colon should be invalid");
    require(!isValidRenameName(L"a*b"), "asterisk should be invalid");
    require(!isValidRenameName(L"a?b"), "question mark should be invalid");
    require(!isValidRenameName(L"a\"b"), "quote should be invalid");
    require(!isValidRenameName(L"a<b"), "less-than should be invalid");
    require(!isValidRenameName(L"a>b"), "greater-than should be invalid");
    require(!isValidRenameName(L"a|b"), "pipe should be invalid");

    std::cout << "RenameDialogTests passed\n";
    return 0;
}
```

- [ ] **Step 4: Create pending operation state API**

Add `src/ui/FileOperationState.h`:

```cpp
#pragma once

#include <string>

namespace finderx::ui {

enum class PendingFileOperationKind {
    None,
    Copy,
    Cut
};

class FileOperationState {
public:
    void setCopy(std::wstring path);
    void setCut(std::wstring path);
    void clear();
    void markPasteSucceeded();

    bool hasPendingOperation() const;
    PendingFileOperationKind kind() const;
    const std::wstring& sourcePath() const;

private:
    PendingFileOperationKind kind_ = PendingFileOperationKind::None;
    std::wstring sourcePath_;
};

}
```

- [ ] **Step 5: Implement pending operation state**

Add `src/ui/FileOperationState.cpp`:

```cpp
#include "ui/FileOperationState.h"

#include <utility>

namespace finderx::ui {

void FileOperationState::setCopy(std::wstring path) {
    if (path.empty()) {
        clear();
        return;
    }
    kind_ = PendingFileOperationKind::Copy;
    sourcePath_ = std::move(path);
}

void FileOperationState::setCut(std::wstring path) {
    if (path.empty()) {
        clear();
        return;
    }
    kind_ = PendingFileOperationKind::Cut;
    sourcePath_ = std::move(path);
}

void FileOperationState::clear() {
    kind_ = PendingFileOperationKind::None;
    sourcePath_.clear();
}

void FileOperationState::markPasteSucceeded() {
    if (kind_ == PendingFileOperationKind::Cut) {
        clear();
    }
}

bool FileOperationState::hasPendingOperation() const {
    return kind_ != PendingFileOperationKind::None && !sourcePath_.empty();
}

PendingFileOperationKind FileOperationState::kind() const {
    return kind_;
}

const std::wstring& FileOperationState::sourcePath() const {
    return sourcePath_;
}

}
```

- [ ] **Step 6: Create pending operation tests**

Add `src/ui/FileOperationStateTests.cpp`:

```cpp
#include "ui/FileOperationState.h"

#include <cstdlib>
#include <iostream>

using namespace finderx::ui;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

int main() {
    FileOperationState state;
    require(!state.hasPendingOperation(), "new state should be empty");

    state.setCopy(L"C:\\Temp\\a.txt");
    require(state.hasPendingOperation(), "copy should set pending operation");
    require(state.kind() == PendingFileOperationKind::Copy, "copy kind should be stored");
    require(state.sourcePath() == L"C:\\Temp\\a.txt", "copy source path should be stored");
    state.markPasteSucceeded();
    require(state.hasPendingOperation(), "copy should remain after paste succeeds");

    state.setCut(L"C:\\Temp\\b.txt");
    require(state.hasPendingOperation(), "cut should set pending operation");
    require(state.kind() == PendingFileOperationKind::Cut, "cut kind should be stored");
    state.markPasteSucceeded();
    require(!state.hasPendingOperation(), "cut should clear after paste succeeds");

    state.setCopy(L"");
    require(!state.hasPendingOperation(), "empty copy path should clear state");
    state.setCut(L"");
    require(!state.hasPendingOperation(), "empty cut path should clear state");

    std::cout << "FileOperationStateTests passed\n";
    return 0;
}
```

- [ ] **Step 7: Wire tests into CMake**

Modify `CMakeLists.txt` after the navigation test target:

```cmake
add_executable(finderx_rename_dialog_tests
    src/ui/RenameDialogTests.cpp
    src/ui/RenameDialog.cpp
)

target_include_directories(finderx_rename_dialog_tests PRIVATE src)
finderx_enable_utf8(finderx_rename_dialog_tests)
finderx_enable_windows_unicode(finderx_rename_dialog_tests)
add_test(NAME finderx_rename_dialog_tests COMMAND finderx_rename_dialog_tests)

add_executable(finderx_file_operation_state_tests
    src/ui/FileOperationStateTests.cpp
    src/ui/FileOperationState.cpp
)

target_include_directories(finderx_file_operation_state_tests PRIVATE src)
finderx_enable_utf8(finderx_file_operation_state_tests)
finderx_enable_windows_unicode(finderx_file_operation_state_tests)
add_test(NAME finderx_file_operation_state_tests COMMAND finderx_file_operation_state_tests)
```

- [ ] **Step 8: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: build succeeds and 5 tests pass.

- [ ] **Step 9: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/ui/RenameDialog.h src/ui/RenameDialog.cpp src/ui/RenameDialogTests.cpp src/ui/FileOperationState.h src/ui/FileOperationState.cpp src/ui/FileOperationStateTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add file operation helpers"
```

---

### Task 2: Shell File Operations

**Files:**
- Create: `src/shell/ShellFileOperations.h`
- Create: `src/shell/ShellFileOperations.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create shell operation API**

Add `src/shell/ShellFileOperations.h`:

```cpp
#pragma once

#include <windows.h>

#include <string>

namespace finderx::shell {

struct FileOperationResult {
    bool success = false;
    std::wstring message;
    std::wstring resultingPath;
};

FileOperationResult renamePath(HWND owner, const std::wstring& oldPath, const std::wstring& newName);
FileOperationResult moveToTrash(HWND owner, const std::wstring& path);
FileOperationResult copyToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory);
FileOperationResult moveToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory);

}
```

- [ ] **Step 2: Implement shell operation helpers**

Add `src/shell/ShellFileOperations.cpp`:

```cpp
#include "shell/ShellFileOperations.h"

#include "ui/RenameDialog.h"

#include <filesystem>
#include <shellapi.h>
#include <utility>

namespace finderx::shell {
namespace {

FileOperationResult failure(std::wstring message) {
    FileOperationResult result;
    result.message = std::move(message);
    return result;
}

FileOperationResult success(std::wstring resultingPath = {}) {
    FileOperationResult result;
    result.success = true;
    result.resultingPath = std::move(resultingPath);
    return result;
}

std::wstring parentPathOf(const std::wstring& path) {
    return std::filesystem::path(path).parent_path().wstring();
}

std::wstring combinePath(const std::wstring& directory, const std::wstring& name) {
    return (std::filesystem::path(directory) / name).wstring();
}

std::wstring doubleNullTerminated(const std::wstring& path) {
    std::wstring value = path;
    value.push_back(L'\0');
    value.push_back(L'\0');
    return value;
}

FileOperationResult runShellOperation(HWND owner, UINT operation, const std::wstring& from,
                                      const std::wstring& to, const std::wstring& failureMessage) {
    if (from.empty()) {
        return failure(failureMessage);
    }

    SHFILEOPSTRUCTW fileOperation{};
    fileOperation.hwnd = owner;
    fileOperation.wFunc = operation;
    const std::wstring fromBuffer = doubleNullTerminated(from);
    const std::wstring toBuffer = to.empty() ? std::wstring{} : doubleNullTerminated(to);
    fileOperation.pFrom = fromBuffer.c_str();
    fileOperation.pTo = toBuffer.empty() ? nullptr : toBuffer.c_str();
    fileOperation.fFlags = FOF_ALLOWUNDO;

    const int code = SHFileOperationW(&fileOperation);
    if (code != 0 || fileOperation.fAnyOperationsAborted) {
        return failure(failureMessage);
    }
    return success();
}

}

FileOperationResult renamePath(HWND, const std::wstring& oldPath, const std::wstring& newName) {
    if (oldPath.empty() || !finderx::ui::isValidRenameName(newName)) {
        return failure(L"Cannot rename item");
    }

    const std::wstring parent = parentPathOf(oldPath);
    const std::wstring newPath = combinePath(parent, newName);
    std::error_code error;
    std::filesystem::rename(oldPath, newPath, error);
    if (error) {
        return failure(L"Cannot rename item");
    }
    return success(newPath);
}

FileOperationResult moveToTrash(HWND owner, const std::wstring& path) {
    return runShellOperation(owner, FO_DELETE, path, L"", L"Cannot move item to trash");
}

FileOperationResult copyToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory) {
    if (destinationDirectory.empty()) {
        return failure(L"Cannot paste here");
    }
    return runShellOperation(owner, FO_COPY, sourcePath, destinationDirectory, L"Cannot copy item");
}

FileOperationResult moveToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory) {
    if (destinationDirectory.empty()) {
        return failure(L"Cannot paste here");
    }
    return runShellOperation(owner, FO_MOVE, sourcePath, destinationDirectory, L"Cannot move item");
}

}
```

- [ ] **Step 3: Add source to CMake**

Modify `FINDERX_APP_SOURCES` in `CMakeLists.txt`:

```cmake
set(FINDERX_APP_SOURCES
    src/main.cpp
    src/app/App.cpp
    src/shell/ShellActions.cpp
    src/shell/ShellFileOperations.cpp
    src/ui/FileOperationState.cpp
    src/ui/FinderChrome.cpp
    src/ui/FinderListView.cpp
    src/ui/MainWindow.cpp
    src/ui/RenameDialog.cpp
    src/ui/RenderContext.cpp
)
```

- [ ] **Step 4: Build**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX
```

Expected: `FinderX.exe` builds successfully.

- [ ] **Step 5: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/shell/ShellFileOperations.h src/shell/ShellFileOperations.cpp
git -c safe.directory=D:/finderX commit -m "feat: add shell file operations"
```

---

### Task 3: Native Rename Dialog

**Files:**
- Modify: `src/ui/RenameDialog.cpp`

- [ ] **Step 1: Replace the dialog stub with a modal Win32 dialog window**

Modify `src/ui/RenameDialog.cpp` so `promptForRename` creates a small owner-modal popup with an edit control, OK button, and Cancel button. Keep `isValidRenameName` unchanged.

Use this implementation shape:

```cpp
struct RenameDialogState {
    std::wstring currentName;
    std::wstring result;
    bool accepted = false;
};

constexpr int kEditId = 101;
constexpr int kOkId = IDOK;
constexpr int kCancelId = IDCANCEL;

LRESULT CALLBACK renameDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<RenameDialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        state = static_cast<RenameDialogState*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }

    switch (message) {
    case WM_CREATE:
        CreateWindowExW(0, L"STATIC", L"Name:", WS_CHILD | WS_VISIBLE, 14, 16, 52, 22, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", state->currentName.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                        64, 12, 300, 26, hwnd, reinterpret_cast<HMENU>(kEditId), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                        204, 52, 76, 28, hwnd, reinterpret_cast<HMENU>(kOkId), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                        288, 52, 76, 28, hwnd, reinterpret_cast<HMENU>(kCancelId), nullptr, nullptr);
        SetFocus(GetDlgItem(hwnd, kEditId));
        SendMessageW(GetDlgItem(hwnd, kEditId), EM_SETSEL, 0, -1);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == kOkId) {
            wchar_t buffer[MAX_PATH]{};
            GetWindowTextW(GetDlgItem(hwnd, kEditId), buffer, MAX_PATH);
            state->result = buffer;
            state->accepted = isValidRenameName(state->result);
            if (state->accepted) {
                DestroyWindow(hwnd);
            } else {
                MessageBoxW(hwnd, L"Enter a valid file name.", L"Rename", MB_ICONWARNING | MB_OK);
            }
            return 0;
        }
        if (LOWORD(wParam) == kCancelId) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
```

`promptForRename` should:

1. Register a `FinderXRenameDialog` class.
2. Disable the owner window with `EnableWindow(owner, FALSE)` while open.
3. Create the dialog centered near the owner.
4. Run a local message loop until the dialog is destroyed.
5. Re-enable and focus the owner.
6. Return `true` only when the user accepted a valid name and the name differs from `currentName`.

- [ ] **Step 2: Build and run validation tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_rename_dialog_tests
& "D:\finderX\build\Debug\finderx_rename_dialog_tests.exe"
```

Expected: build succeeds and `RenameDialogTests passed`.

- [ ] **Step 3: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/RenameDialog.cpp
git -c safe.directory=D:/finderX commit -m "feat: add rename dialog"
```

---

### Task 4: MainWindow File Operation Integration

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add includes and state to `MainWindow.h`**

Add:

```cpp
#include "ui/FileOperationState.h"
```

Add private methods:

```cpp
    NodeId commandTargetNode() const;
    void renameContextNode();
    void moveContextNodeToTrash();
    void copyContextNode();
    void cutContextNode();
    void pasteIntoCurrentDirectory();
    bool refreshCurrentDirectorySelecting(const std::wstring& selectedPath);
```

Add member:

```cpp
    ui::FileOperationState fileOperationState_;
```

- [ ] **Step 2: Add command ids and includes to `MainWindow.cpp`**

Add:

```cpp
#include "shell/ShellFileOperations.h"
#include "ui/RenameDialog.h"
```

Add command ids:

```cpp
constexpr UINT kCommandRename = 1005;
constexpr UINT kCommandCopy = 1006;
constexpr UINT kCommandCut = 1007;
constexpr UINT kCommandPaste = 1008;
constexpr UINT kCommandMoveToTrash = 1009;
```

- [ ] **Step 3: Extend context menu**

Replace the row menu construction inside `showContextMenu` with:

```cpp
    if (hasTarget) {
        AppendMenuW(menu, MF_STRING, kCommandOpen, L"Open");
        AppendMenuW(menu, MF_STRING, kCommandRename, L"Rename");
        AppendMenuW(menu, MF_STRING, kCommandCopy, L"Copy");
        AppendMenuW(menu, MF_STRING, kCommandCut, L"Cut");
        if (fileOperationState_.hasPendingOperation()) {
            AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
        }
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kCommandReveal, L"Show in Explorer");
        AppendMenuW(menu, MF_STRING, kCommandCopyPath, L"Copy Path");
        AppendMenuW(menu, MF_STRING, kCommandMoveToTrash, L"Move to Trash");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    } else if (fileOperationState_.hasPendingOperation()) {
        AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }
    AppendMenuW(menu, MF_STRING, kCommandRefresh, L"Refresh");
```

- [ ] **Step 4: Add keyboard shortcuts**

At the start of the `WM_KEYDOWN` case, before refresh/back/list handling:

```cpp
        contextNode_ = kInvalidNodeId;
        const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (wParam == VK_F2) {
            renameContextNode();
            return 0;
        }
        if (wParam == VK_DELETE) {
            moveContextNodeToTrash();
            return 0;
        }
        if (controlDown && wParam == L'C') {
            copyContextNode();
            return 0;
        }
        if (controlDown && wParam == L'X') {
            cutContextNode();
            return 0;
        }
        if (controlDown && wParam == L'V') {
            pasteIntoCurrentDirectory();
            return 0;
        }
```

Update the existing Ctrl+R check to use `controlDown`.

- [ ] **Step 5: Extend command dispatch**

Add cases to `handleCommand`:

```cpp
    case kCommandRename:
        renameContextNode();
        break;
    case kCommandCopy:
        copyContextNode();
        break;
    case kCommandCut:
        cutContextNode();
        break;
    case kCommandPaste:
        pasteIntoCurrentDirectory();
        break;
    case kCommandMoveToTrash:
        moveContextNodeToTrash();
        break;
```

- [ ] **Step 6: Implement target and refresh helpers**

Add:

```cpp
NodeId MainWindow::commandTargetNode() const {
    if (contextNode_ != kInvalidNodeId && contextNode_ < tree_.nodes().size()) {
        return contextNode_;
    }
    return listView_.selectedNode();
}

bool MainWindow::refreshCurrentDirectorySelecting(const std::wstring& selectedPath) {
    const std::wstring previousSelection = selectedPath;
    const std::wstring currentPath = history_.currentPath();
    if (currentPath.empty()) {
        setStatusText(L"Cannot refresh folder");
        return false;
    }

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
    contextNode_ = kInvalidNodeId;
    chromeState_.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}
```

Then update `refreshCurrentDirectory()` to call:

```cpp
return refreshCurrentDirectorySelecting(selectedNodePath());
```

- [ ] **Step 7: Implement rename/copy/cut/delete/paste handlers**

Add:

```cpp
void MainWindow::renameContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    const FileNode& node = tree_.node(target);
    std::wstring newName;
    if (!ui::promptForRename(hwnd_, node.name, newName)) {
        return;
    }

    const shell::FileOperationResult result = shell::renamePath(hwnd_, node.path, newName);
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot rename item" : result.message);
        return;
    }
    refreshCurrentDirectorySelecting(result.resultingPath);
}

void MainWindow::moveContextNodeToTrash() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }

    const shell::FileOperationResult result = shell::moveToTrash(hwnd_, tree_.node(target).path);
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot move item to trash" : result.message);
        return;
    }
    refreshCurrentDirectorySelecting(L"");
}

void MainWindow::copyContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }
    fileOperationState_.setCopy(tree_.node(target).path);
}

void MainWindow::cutContextNode() {
    const NodeId target = commandTargetNode();
    if (target == kInvalidNodeId || target >= tree_.nodes().size()) {
        return;
    }
    fileOperationState_.setCut(tree_.node(target).path);
}

void MainWindow::pasteIntoCurrentDirectory() {
    if (!fileOperationState_.hasPendingOperation()) {
        return;
    }

    const std::wstring destination = history_.currentPath();
    if (destination.empty()) {
        setStatusText(L"Cannot paste here");
        return;
    }

    shell::FileOperationResult result;
    if (fileOperationState_.kind() == ui::PendingFileOperationKind::Copy) {
        result = shell::copyToDirectory(hwnd_, fileOperationState_.sourcePath(), destination);
    } else {
        result = shell::moveToDirectory(hwnd_, fileOperationState_.sourcePath(), destination);
    }

    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot paste here" : result.message);
        return;
    }

    fileOperationState_.markPasteSucceeded();
    refreshCurrentDirectory();
}
```

- [ ] **Step 8: Build and run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: build succeeds and 5 tests pass.

- [ ] **Step 9: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: wire file operations into main window"
```

---

### Task 5: Final Verification And Manual Smoke Check

**Files:**
- No planned source edits.

- [ ] **Step 1: Configure/build Debug and Release**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

Expected: both builds succeed.

- [ ] **Step 2: Run automated tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: 5/5 tests pass.

- [ ] **Step 3: Run diff check**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
```

Expected: no output and exit code 0.

- [ ] **Step 4: Launch smoke test**

Run:

```powershell
$p = Start-Process -FilePath "D:\finderX\build\Debug\FinderX.exe" -PassThru
Start-Sleep -Seconds 3
$exited = $p.HasExited
if (-not $exited) {
    $p.CloseMainWindow() | Out-Null
    Start-Sleep -Milliseconds 500
    if (-not $p.HasExited) { $p.Kill() }
}
if ($exited) {
    Write-Error "FinderX exited during smoke test"
    exit 1
}
Write-Output "FinderX launch smoke passed"
```

Expected: `FinderX launch smoke passed`.

- [ ] **Step 5: Manual UI checks on disposable test files**

Create a disposable folder under the user's home directory, then manually verify:

- F2 opens rename dialog for a selected file.
- Rename rejects `bad:name.txt`.
- Rename accepts `renamed.txt` and keeps the renamed item selected after refresh.
- Delete moves a disposable file to Recycle Bin.
- Ctrl+C then Ctrl+V copies a disposable file into the current directory, using Windows conflict UI if the name already exists.
- Ctrl+X then Ctrl+V moves a disposable file and clears cut state.
- Right-click row shows Rename, Copy, Cut, Paste when pending, Move to Trash, existing Open, Show in Explorer, Copy Path, and Refresh.
- Right-click blank list space shows Paste when pending, then Refresh.
- Back/Forward history is unchanged after rename/delete/paste.

- [ ] **Step 6: Commit any verification-only fixes**

If manual smoke finds a defect, fix the defect in the smallest relevant file, rerun Steps 1-4, then commit:

```powershell
git -c safe.directory=D:/finderX add src/shell/ShellFileOperations.h src/shell/ShellFileOperations.cpp src/ui/FileOperationState.h src/ui/FileOperationState.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp src/ui/RenameDialog.h src/ui/RenameDialog.cpp CMakeLists.txt
git -c safe.directory=D:/finderX commit -m "fix: polish file operation behavior"
```

If no source changes are needed, do not create a commit for this task.
