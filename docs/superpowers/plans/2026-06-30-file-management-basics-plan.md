# FinderX File Management Basics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first everyday file-management commands: create folder, create file, open PowerShell here, hide known-empty folder arrows, and browse drives from a `This PC` sidebar entry.

**Architecture:** Keep filesystem creation and drive enumeration in `src/fs`, keep shell launching in `src/shell`, and keep UI state/routing in `MainWindow`. `This PC` is represented as a lightweight virtual location identified by a constant path, while real directories still use `DirectoryLoader`.

**Tech Stack:** C++20, Win32 API, Direct2D/DirectWrite UI, CMake/CTest, existing FinderX model/navigation/ui libraries.

---

## File Structure

- Create `src/fs/FileCreation.h` and `src/fs/FileCreation.cpp`
  - Responsible for default names, conflict-free names, `CreateDirectoryW`, and `CreateFileW` with `CREATE_NEW`.
- Modify `src/fs/DirectoryLoaderTests.cpp`
  - Add focused temp-directory tests for new folder/file name generation and creation behavior.
- Create `src/fs/DriveEnumerator.h` and `src/fs/DriveEnumerator.cpp`
  - Responsible for turning Windows logical drives into `FileNode` rows and exposing a pure mask-to-roots helper for stable tests.
- Create `src/fs/DriveEnumeratorTests.cpp`
  - Test mask parsing without assuming a specific machine has `D:\`.
- Modify `src/shell/ShellActions.h` and `src/shell/ShellActions.cpp`
  - Add `openPowerShellAt(HWND, const std::wstring&)` using `ShellExecuteW` working-directory parameter.
- Modify `src/navigation/SidebarModel.h`, `src/navigation/SidebarModel.cpp`, and `src/navigation/NavigationHistoryTests.cpp`
  - Add `thisPcPath()`, include `This PC` in sidebar rows, and mark it selected for the virtual location.
- Modify `src/ui/FinderListView.cpp` and `src/ui/FinderListViewTests.cpp`
  - Add a single helper that controls whether a folder can show/toggle disclosure, then test known-empty folder behavior.
- Modify `src/ui/MainWindow.h` and `src/ui/MainWindow.cpp`
  - Add virtual location state, right-click commands, creation handlers, PowerShell target selection, and `This PC` navigation.
- Modify `src/ui/FinderChrome.cpp`
  - Draw a distinct sidebar icon for `This PC`.
- Modify `CMakeLists.txt`
  - Add new source files to `finderx_fs` and `FinderX`.
  - Add `finderx_drive_enumerator_tests`.

---

### Task 1: Filesystem Creation Helpers

**Files:**
- Create: `src/fs/FileCreation.h`
- Create: `src/fs/FileCreation.cpp`
- Modify: `src/fs/DirectoryLoaderTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add failing tests for default names, conflicts, and creation**

Append these includes near the top of `src/fs/DirectoryLoaderTests.cpp`:

```cpp
#include "fs/FileCreation.h"

#include <fstream>
```

Add this helper inside the anonymous namespace in `src/fs/DirectoryLoaderTests.cpp`:

```cpp
std::wstring uniqueTempChild(const std::wstring& prefix) {
    wchar_t tempPath[MAX_PATH]{};
    const DWORD length = GetTempPathW(static_cast<DWORD>(std::size(tempPath)), tempPath);
    require(length > 0 && length < std::size(tempPath), "temp path should be available");

    std::wstring base(tempPath);
    if (!base.empty() && base.back() != L'\\') {
        base.push_back(L'\\');
    }
    base += prefix + std::to_wstring(GetTickCount64());
    return base;
}

void removeTreeIfExists(const std::wstring& path) {
    std::filesystem::remove_all(std::filesystem::path(path));
}
```

Add this test function before `main()`:

```cpp
void testFileCreationHelpers() {
    const std::wstring root = uniqueTempChild(L"finderx-create-");
    removeTreeIfExists(root);
    require(CreateDirectoryW(root.c_str(), nullptr) != 0, "creation test directory should be created");

    const std::wstring firstFolder = nextNewFolderPath(root);
    require(firstFolder == root + L"\\New Folder", "first folder name should use base name");
    require(CreateDirectoryW(firstFolder.c_str(), nullptr) != 0, "base folder should be created");

    const std::wstring secondFolder = nextNewFolderPath(root);
    require(secondFolder == root + L"\\New Folder 2", "second folder name should use numeric suffix");

    const FileCreationResult folderResult = createNewFolder(root);
    require(folderResult.success, "folder creation should succeed");
    require(folderResult.path == secondFolder, "folder creation should use next folder name");
    require(std::filesystem::is_directory(std::filesystem::path(secondFolder)), "created folder should exist");

    const std::wstring firstFile = nextNewTextFilePath(root);
    require(firstFile == root + L"\\New Text Document.txt", "first file name should use base name");
    {
        std::ofstream stream(std::filesystem::path(firstFile));
        require(stream.good(), "base file should be created");
    }

    const std::wstring secondFile = nextNewTextFilePath(root);
    require(secondFile == root + L"\\New Text Document 2.txt", "second file name should use numeric suffix before extension");

    const FileCreationResult fileResult = createNewTextFile(root);
    require(fileResult.success, "file creation should succeed");
    require(fileResult.path == secondFile, "file creation should use next file name");
    require(std::filesystem::is_regular_file(std::filesystem::path(secondFile)), "created file should exist");

    removeTreeIfExists(root);
}
```

Call it from `main()`:

```cpp
int main() {
    testDirectoryLoader();
    testFileCreationHelpers();
    std::cout << "DirectoryLoaderTests passed\n";
}
```

- [ ] **Step 2: Run the filesystem tests and verify they fail because the helper does not exist**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_fs_tests
```

Expected: compile failure mentioning `fs/FileCreation.h` or undefined `nextNewFolderPath`.

- [ ] **Step 3: Add the creation helper API**

Create `src/fs/FileCreation.h`:

```cpp
#pragma once

#include <string>

namespace finderx {

struct FileCreationResult {
    bool success = false;
    std::wstring path;
    std::wstring message;
};

std::wstring nextNewFolderPath(const std::wstring& directory);
std::wstring nextNewTextFilePath(const std::wstring& directory);
FileCreationResult createNewFolder(const std::wstring& directory);
FileCreationResult createNewTextFile(const std::wstring& directory);

} // namespace finderx
```

Create `src/fs/FileCreation.cpp`:

```cpp
#include "fs/FileCreation.h"

#include <windows.h>

namespace finderx {
namespace {

std::wstring joinPath(const std::wstring& directory, const std::wstring& child) {
    if (directory.empty()) {
        return child;
    }

    const wchar_t last = directory.back();
    if (last == L'\\' || last == L'/') {
        return directory + child;
    }

    return directory + L"\\" + child;
}

bool pathExists(const std::wstring& path) {
    return !path.empty() && GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring numberedName(const std::wstring& baseName, const std::wstring& extension, int index) {
    if (index == 1) {
        return baseName + extension;
    }
    return baseName + L" " + std::to_wstring(index) + extension;
}

std::wstring nextAvailablePath(
    const std::wstring& directory,
    const std::wstring& baseName,
    const std::wstring& extension) {
    for (int index = 1; index < 10000; ++index) {
        const std::wstring candidate = joinPath(directory, numberedName(baseName, extension, index));
        if (!pathExists(candidate)) {
            return candidate;
        }
    }
    return {};
}

FileCreationResult failure(std::wstring message) {
    FileCreationResult result;
    result.message = std::move(message);
    return result;
}

} // namespace

std::wstring nextNewFolderPath(const std::wstring& directory) {
    return nextAvailablePath(directory, L"New Folder", L"");
}

std::wstring nextNewTextFilePath(const std::wstring& directory) {
    return nextAvailablePath(directory, L"New Text Document", L".txt");
}

FileCreationResult createNewFolder(const std::wstring& directory) {
    const std::wstring path = nextNewFolderPath(directory);
    if (path.empty()) {
        return failure(L"Cannot create folder");
    }

    if (CreateDirectoryW(path.c_str(), nullptr) == 0) {
        return failure(L"Cannot create folder");
    }

    FileCreationResult result;
    result.success = true;
    result.path = path;
    return result;
}

FileCreationResult createNewTextFile(const std::wstring& directory) {
    const std::wstring path = nextNewTextFilePath(directory);
    if (path.empty()) {
        return failure(L"Cannot create file");
    }

    const HANDLE handle = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return failure(L"Cannot create file");
    }

    CloseHandle(handle);

    FileCreationResult result;
    result.success = true;
    result.path = path;
    return result;
}

} // namespace finderx
```

- [ ] **Step 4: Wire the helper into CMake**

Modify the `finderx_fs` library in `CMakeLists.txt`:

```cmake
add_library(finderx_fs
    src/fs/FileMetadata.cpp
    src/fs/DirectoryLoader.cpp
    src/fs/FileCreation.cpp
)
```

- [ ] **Step 5: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_fs_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_fs_tests --output-on-failure
```

Expected: build succeeds and `finderx_fs_tests` passes.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/fs/FileCreation.h src/fs/FileCreation.cpp src/fs/DirectoryLoaderTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add file creation helpers"
```

---

### Task 2: Drive Enumeration And This PC Sidebar Model

**Files:**
- Create: `src/fs/DriveEnumerator.h`
- Create: `src/fs/DriveEnumerator.cpp`
- Create: `src/fs/DriveEnumeratorTests.cpp`
- Modify: `src/navigation/SidebarModel.h`
- Modify: `src/navigation/SidebarModel.cpp`
- Modify: `src/navigation/NavigationHistoryTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add failing drive and sidebar tests**

Create `src/fs/DriveEnumeratorTests.cpp`:

```cpp
#include "fs/DriveEnumerator.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    const std::vector<std::wstring> roots = driveRootsFromMaskForTests((1u << 0) | (1u << 2) | (1u << 25));
    require(roots.size() == 3, "mask should produce three roots");
    require(roots[0] == L"A:\\", "first root should be A");
    require(roots[1] == L"C:\\", "second root should be C");
    require(roots[2] == L"Z:\\", "third root should be Z");

    const std::vector<FileNode> nodes = driveNodesFromRootsForTests({L"C:\\", L"D:\\"});
    require(nodes.size() == 2, "roots should produce two nodes");
    require(nodes[0].name == L"C:\\", "node name should be drive root");
    require(nodes[0].path == L"C:\\", "node path should be drive root");
    require(nodes[0].kind == FileKind::Folder, "drive node should be folder-like");
    require(nodes[0].childrenLoaded == false, "drive node children should load on navigation");

    std::cout << "DriveEnumeratorTests passed\n";
}
```

In `src/navigation/NavigationHistoryTests.cpp`, extend `testSidebarModel()` after the existing item checks:

```cpp
    const SidebarItem& thisPc = findItem(model, L"This PC");
    require(thisPc.path == thisPcPath(), "This PC should use virtual path");
    require(thisPc.available, "This PC should always be available");

    model.refresh(L"C:\\Users\\Example", thisPcPath());
    require(findItem(model, L"This PC").selected, "This PC should be selected for virtual path");
```

- [ ] **Step 2: Run tests and verify they fail because APIs do not exist**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_navigation_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_drive_enumerator_tests
```

Expected: compile failure for `thisPcPath` and missing `finderx_drive_enumerator_tests` target until CMake is updated.

- [ ] **Step 3: Add drive enumeration helpers**

Create `src/fs/DriveEnumerator.h`:

```cpp
#pragma once

#include "model/FileNode.h"

#include <string>
#include <vector>

namespace finderx {

std::vector<std::wstring> enumerateDriveRoots();
std::vector<FileNode> enumerateDriveNodes();
std::vector<std::wstring> driveRootsFromMaskForTests(unsigned long mask);
std::vector<FileNode> driveNodesFromRootsForTests(const std::vector<std::wstring>& roots);

} // namespace finderx
```

Create `src/fs/DriveEnumerator.cpp`:

```cpp
#include "fs/DriveEnumerator.h"

#include <windows.h>

namespace finderx {
namespace {

std::vector<std::wstring> driveRootsFromMask(unsigned long mask) {
    std::vector<std::wstring> roots;
    for (int index = 0; index < 26; ++index) {
        if ((mask & (1ul << index)) == 0) {
            continue;
        }
        std::wstring root;
        root.push_back(static_cast<wchar_t>(L'A' + index));
        root += L":\\";
        roots.push_back(std::move(root));
    }
    return roots;
}

FileNode driveNode(const std::wstring& root) {
    FileNode node;
    node.name = root;
    node.path = root;
    node.kind = FileKind::Folder;
    node.modified = L"--";
    node.sizeText = L"--";
    node.kindText = L"Drive";
    node.expanded = false;
    node.childrenLoaded = false;
    return node;
}

} // namespace

std::vector<std::wstring> enumerateDriveRoots() {
    return driveRootsFromMask(GetLogicalDrives());
}

std::vector<FileNode> enumerateDriveNodes() {
    return driveNodesFromRootsForTests(enumerateDriveRoots());
}

std::vector<std::wstring> driveRootsFromMaskForTests(unsigned long mask) {
    return driveRootsFromMask(mask);
}

std::vector<FileNode> driveNodesFromRootsForTests(const std::vector<std::wstring>& roots) {
    std::vector<FileNode> nodes;
    nodes.reserve(roots.size());
    for (const std::wstring& root : roots) {
        nodes.push_back(driveNode(root));
    }
    return nodes;
}

} // namespace finderx
```

- [ ] **Step 4: Add This PC to the sidebar model**

Modify `src/navigation/SidebarModel.h`:

```cpp
std::wstring thisPcPath();
```

Modify `src/navigation/SidebarModel.cpp` by adding this function after `userProgramsDirectory()`:

```cpp
std::wstring thisPcPath() {
    return L"finderx://this-pc";
}
```

Add this helper in the anonymous namespace:

```cpp
SidebarItem makeThisPcItem(const std::wstring& currentPath) {
    SidebarItem item;
    item.label = L"This PC";
    item.path = thisPcPath();
    item.available = true;
    item.selected = pathsEqual(item.path, currentPath);
    return item;
}
```

Change `SidebarModel::refresh`:

```cpp
void SidebarModel::refresh(const std::wstring& homePath, const std::wstring& currentPath) {
    items_.clear();
    items_.reserve(6);
    items_.push_back(makeItem(L"Home", homePath, currentPath));
    items_.push_back(makeItem(L"Desktop", joinPathForNavigation(homePath, L"Desktop"), currentPath));
    items_.push_back(makeItem(L"Documents", joinPathForNavigation(homePath, L"Documents"), currentPath));
    items_.push_back(makeItem(L"Downloads", joinPathForNavigation(homePath, L"Downloads"), currentPath));
    items_.push_back(makeThisPcItem(currentPath));
    items_.push_back(makeItem(L"Applications", userProgramsDirectory(), currentPath));
}
```

- [ ] **Step 5: Wire drive tests into CMake**

Modify `finderx_fs`:

```cmake
add_library(finderx_fs
    src/fs/FileMetadata.cpp
    src/fs/DirectoryLoader.cpp
    src/fs/FileCreation.cpp
    src/fs/DriveEnumerator.cpp
)
```

Add this test block after `finderx_fs_tests`:

```cmake
add_executable(finderx_drive_enumerator_tests
    src/fs/DriveEnumeratorTests.cpp
)

target_link_libraries(finderx_drive_enumerator_tests PRIVATE finderx_fs)
finderx_enable_utf8(finderx_drive_enumerator_tests)
finderx_enable_windows_unicode(finderx_drive_enumerator_tests)
add_test(NAME finderx_drive_enumerator_tests COMMAND finderx_drive_enumerator_tests)
```

- [ ] **Step 6: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_drive_enumerator_tests finderx_navigation_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R "finderx_drive_enumerator_tests|finderx_navigation_tests" --output-on-failure
```

Expected: both tests pass.

- [ ] **Step 7: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/fs/DriveEnumerator.h src/fs/DriveEnumerator.cpp src/fs/DriveEnumeratorTests.cpp src/navigation/SidebarModel.h src/navigation/SidebarModel.cpp src/navigation/NavigationHistoryTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add this pc sidebar model"
```

---

### Task 3: Hide Known-Empty Folder Disclosure Arrows

**Files:**
- Modify: `src/ui/FinderListView.cpp`
- Modify: `src/ui/FinderListViewTests.cpp`

- [ ] **Step 1: Add failing list-view test for known-empty folders**

Append this test block near the other mouse interaction tests in `src/ui/FinderListViewTests.cpp`:

```cpp
        FileTree tree(L"C:\\Root", L"Root");
        FileNode emptyFolder;
        emptyFolder.name = L"Empty";
        emptyFolder.path = L"C:\\Root\\Empty";
        emptyFolder.kind = FileKind::Folder;
        tree.replaceChildren(tree.rootId(), {emptyFolder});

        const NodeId emptyId = tree.node(tree.rootId()).children.front();
        tree.replaceChildren(emptyId, {});

        FinderListView view(&tree);
        const D2D1_RECT_F bounds = D2D1::RectF(0.0f, 0.0f, 800.0f, 400.0f);
        const ListInteractionResult result = view.onMouseDown(8.0f, 10.0f, bounds, false, false);

        require(result.expandedFolder == kInvalidNodeId, "known-empty folder should not request child loading");
        require(!tree.node(emptyId).expanded, "known-empty folder should not expand from disclosure click");
```

- [ ] **Step 2: Run the list-view test and verify it fails**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_list_view_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_list_view_tests --output-on-failure
```

Expected: test fails because the known-empty folder still toggles expansion.

- [ ] **Step 3: Add a single expandable-folder helper and use it for mouse and keyboard expansion**

Add this helper in the anonymous namespace in `src/ui/FinderListView.cpp`:

```cpp
bool isExpandableFolder(const FileNode& node) {
    return node.kind == FileKind::Folder && (!node.childrenLoaded || !node.children.empty());
}
```

Update the disclosure click check:

```cpp
const bool disclosureClick = isExpandableFolder(node) && hitTestDisclosure(x, bounds, row);
```

Update the left-key branch:

```cpp
if (isExpandableFolder(node) && node.expanded) {
    tree_->setExpanded(selected_, false);
    rebuildRows();
    ensureSelection();
    ensureSelectionVisible();
    result.changed = true;
}
```

Update the right-key branch:

```cpp
if (isExpandableFolder(node) && !node.expanded) {
    tree_->setExpanded(selected_, true);
    result.expandedFolder = selected_;
    rebuildRows();
    ensureSelection();
    ensureSelectionVisible();
    result.changed = true;
}
```

Update drawing where `drawDisclosure` is called:

```cpp
if (isExpandableFolder(node) && disclosureX + kDisclosureWidth <= nameRight) {
    drawDisclosure(render, node, disclosureX, y, selected ? selectedTextColor : mutedColor);
}
```

Update any remaining expansion checks in `FinderListView.cpp` that use `node.kind == FileKind::Folder` for arrow-specific behavior to use `isExpandableFolder(node)`.

- [ ] **Step 4: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_list_view_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_list_view_tests --output-on-failure
```

Expected: `finderx_list_view_tests` passes.

- [ ] **Step 5: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp
git -c safe.directory=D:/finderX commit -m "fix: hide known empty folder disclosure"
```

---

### Task 4: Shell PowerShell Action

**Files:**
- Modify: `src/shell/ShellActions.h`
- Modify: `src/shell/ShellActions.cpp`

- [ ] **Step 1: Add the shell API declaration**

Modify `src/shell/ShellActions.h`:

```cpp
bool openPath(HWND owner, const std::wstring& path);
bool revealInExplorer(HWND owner, const std::wstring& path);
bool copyPathToClipboard(HWND owner, const std::wstring& path);
bool openPowerShellAt(HWND owner, const std::wstring& directory);
```

- [ ] **Step 2: Add the implementation**

Add this helper in the anonymous namespace in `src/shell/ShellActions.cpp`:

```cpp
bool directoryExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
```

Add this function after `copyPathToClipboard`:

```cpp
bool openPowerShellAt(HWND owner, const std::wstring& directory) {
    if (!directoryExists(directory)) {
        return false;
    }

    const HINSTANCE result = ShellExecuteW(
        owner,
        L"open",
        L"powershell.exe",
        nullptr,
        directory.c_str(),
        SW_SHOWNORMAL);
    return shellExecuteOk(result);
}
```

- [ ] **Step 3: Build the app**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX
```

Expected: `FinderX` builds successfully.

- [ ] **Step 4: Commit**

```powershell
git -c safe.directory=D:/finderX add src/shell/ShellActions.h src/shell/ShellActions.cpp
git -c safe.directory=D:/finderX commit -m "feat: add powershell launch action"
```

---

### Task 5: MainWindow Integration For Context Commands And This PC

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/ui/FinderChrome.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add new includes and command IDs**

In `src/ui/MainWindow.cpp`, add includes:

```cpp
#include "fs/DriveEnumerator.h"
#include "fs/FileCreation.h"
```

Add command IDs beside existing constants:

```cpp
constexpr UINT kCommandNewFolder = 1010;
constexpr UINT kCommandNewFile = 1011;
constexpr UINT kCommandOpenPowerShell = 1012;
```

- [ ] **Step 2: Extend tab state for virtual locations**

In `src/ui/MainWindow.h`, add this enum inside `MainWindow` private section:

```cpp
enum class LocationKind {
    Directory,
    ThisPc
};
```

Add a field to `TabState`:

```cpp
LocationKind locationKind = LocationKind::Directory;
```

Add method declarations:

```cpp
bool navigateToThisPc(HistoryMode mode);
bool navigateToLocation(std::wstring path, HistoryMode mode);
bool isActiveDirectoryLocation() const;
void createFolderInCurrentDirectory();
void createFileInCurrentDirectory();
void openPowerShellForContext();
std::wstring powerShellTargetDirectory() const;
```

- [ ] **Step 3: Add virtual location navigation**

In `src/ui/MainWindow.cpp`, add:

```cpp
bool MainWindow::navigateToLocation(std::wstring path, HistoryMode mode) {
    if (path == thisPcPath()) {
        return navigateToThisPc(mode);
    }
    return navigateToDirectory(std::move(path), mode);
}

bool MainWindow::navigateToThisPc(HistoryMode mode) {
    if (!hasActiveTab()) {
        tabs_.push_back(std::make_unique<TabState>(thisPcPath(), L"This PC"));
        activeTabIndex_ = 0;
    }

    TabState& tab = activeTab();
    tab.locationKind = LocationKind::ThisPc;
    tab.tree = FileTree(thisPcPath(), L"This PC");
    tab.tree.replaceChildren(tab.tree.rootId(), enumerateDriveNodes());
    tab.listView = FinderListView(&tab.tree);
    tab.searchText.clear();
    tab.searchFocused = false;
    tab.statusText.clear();

    if (mode == HistoryMode::Initial || mode == HistoryMode::Replace) {
        tab.history.setInitialPath(thisPcPath());
    } else if (mode == HistoryMode::Push) {
        tab.history.navigateTo(thisPcPath());
    }

    contextNode_ = kInvalidNodeId;
    stopDirectoryWatcher();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}

bool MainWindow::isActiveDirectoryLocation() const {
    return hasActiveTab() && activeTab().locationKind == LocationKind::Directory;
}
```

At the start of `navigateToDirectory`, after a directory load succeeds and before refreshing chrome, set:

```cpp
tab.locationKind = LocationKind::Directory;
```

Replace sidebar navigation:

```cpp
navigateToLocation(chromeState_.sidebarItems[chromeHit.sidebarIndex].path, HistoryMode::Push);
```

Replace back/forward target navigation:

```cpp
if (!navigateToLocation(target, HistoryMode::BackForward)) {
    return;
}
```

Keep the existing `history.goBack()` and `history.goForward()` calls after successful navigation.

- [ ] **Step 4: Route activation and refresh for This PC**

Modify `activateNode`:

```cpp
void MainWindow::activateNode(NodeId nodeId) {
    TabState& tab = activeTab();
    if (nodeId == kInvalidNodeId || nodeId >= tab.tree.nodes().size()) {
        return;
    }

    const FileNode& node = tab.tree.node(nodeId);
    if (tab.locationKind == LocationKind::ThisPc && node.kind == FileKind::Folder) {
        navigateToDirectory(node.path, HistoryMode::Push);
        return;
    }

    if (node.kind == FileKind::Folder) {
        navigateToDirectory(node.path, HistoryMode::Push);
    } else {
        openFile(node.path);
    }
}
```

At the top of `refreshCurrentDirectorySelecting`:

```cpp
if (!isActiveDirectoryLocation()) {
    if (activeTab().locationKind == LocationKind::ThisPc) {
        return navigateToThisPc(HistoryMode::Replace);
    }
    setStatusText(L"Cannot refresh folder");
    return false;
}
```

In `startDirectoryWatcher`, call it only when `isActiveDirectoryLocation()` is true:

```cpp
if (isActiveDirectoryLocation()) {
    startDirectoryWatcher(activeTab().history.currentPath());
} else {
    stopDirectoryWatcher();
}
```

- [ ] **Step 5: Add context menu items**

Modify `showContextMenu`:

```cpp
const bool directoryLocation = isActiveDirectoryLocation();
```

Inside the target branch, after `Paste` handling and before the separator:

```cpp
if (singleTarget && tab.tree.node(targets.front()).kind == FileKind::Folder) {
    AppendMenuW(menu, MF_STRING, kCommandOpenPowerShell, L"Open in PowerShell");
}
```

Inside the background branch:

```cpp
if (directoryLocation) {
    AppendMenuW(menu, MF_STRING, kCommandNewFolder, L"New Folder");
    AppendMenuW(menu, MF_STRING, kCommandNewFile, L"New File");
    AppendMenuW(menu, MF_STRING, kCommandOpenPowerShell, L"Open in PowerShell");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
}
if (fileOperationState_.hasPendingOperation() && directoryLocation) {
    AppendMenuW(menu, MF_STRING, kCommandPaste, L"Paste");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
}
```

Only append `Refresh` for directory and `This PC` locations:

```cpp
AppendMenuW(menu, MF_STRING, kCommandRefresh, L"Refresh");
```

- [ ] **Step 6: Dispatch commands and implement handlers**

Add cases in `handleCommand`:

```cpp
case kCommandNewFolder:
    createFolderInCurrentDirectory();
    break;
case kCommandNewFile:
    createFileInCurrentDirectory();
    break;
case kCommandOpenPowerShell:
    openPowerShellForContext();
    break;
```

Add methods:

```cpp
void MainWindow::createFolderInCurrentDirectory() {
    if (!isActiveDirectoryLocation()) {
        setStatusText(L"Cannot create folder");
        return;
    }

    const FileCreationResult result = createNewFolder(activeTab().history.currentPath());
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot create folder" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting(result.path);
}

void MainWindow::createFileInCurrentDirectory() {
    if (!isActiveDirectoryLocation()) {
        setStatusText(L"Cannot create file");
        return;
    }

    const FileCreationResult result = createNewTextFile(activeTab().history.currentPath());
    if (!result.success) {
        setStatusText(result.message.empty() ? L"Cannot create file" : result.message);
        return;
    }

    refreshCurrentDirectorySelecting(result.path);
}

std::wstring MainWindow::powerShellTargetDirectory() const {
    if (!isActiveDirectoryLocation()) {
        return {};
    }

    const TabState& tab = activeTab();
    const NodeId target = commandTargetNode();
    if (target != kInvalidNodeId && target < tab.tree.nodes().size()) {
        const FileNode& node = tab.tree.node(target);
        if (node.kind == FileKind::Folder) {
            return node.path;
        }
    }

    return tab.history.currentPath();
}

void MainWindow::openPowerShellForContext() {
    const std::wstring directory = powerShellTargetDirectory();
    if (directory.empty() || !shell::openPowerShellAt(hwnd_, directory)) {
        setStatusText(L"Cannot open PowerShell");
    }
}
```

- [ ] **Step 7: Update paste safeguards for virtual locations**

At the top of `pasteIntoCurrentDirectory`:

```cpp
if (!isActiveDirectoryLocation()) {
    setStatusText(L"Cannot paste here");
    return;
}
```

Keep the existing current-path empty check after this guard.

- [ ] **Step 8: Draw a This PC sidebar icon**

In `src/ui/FinderChrome.cpp`, update `drawSidebarIcon`:

```cpp
if (label == L"This PC") {
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x, y + 2.0f, x + 16.0f, y + 13.0f), 2.0f, 2.0f), color);
    render.fillRect(D2D1::RectF(x + 5.0f, y + 13.0f, x + 11.0f, y + 15.0f), color);
    render.fillRect(D2D1::RectF(x + 3.0f, y + 15.0f, x + 13.0f, y + 16.0f), color);
    return;
}
```

- [ ] **Step 9: Add new app sources to CMake**

Modify `FINDERX_APP_SOURCES`:

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

No app-source entry is needed for `FileCreation.cpp` or `DriveEnumerator.cpp` because `FinderX` links `finderx_fs`.

- [ ] **Step 10: Build and run full tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: Debug build succeeds and all CTest tests pass.

- [ ] **Step 11: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/ui/MainWindow.h src/ui/MainWindow.cpp src/ui/FinderChrome.cpp
git -c safe.directory=D:/finderX commit -m "feat: add file management context actions"
```

---

### Task 6: Manual Smoke Verification And Release Build

**Files:**
- No source changes expected.

- [ ] **Step 1: Build Release**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

Expected: Release build succeeds.

- [ ] **Step 2: Run Release tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 3: Manual app smoke**

Run:

```powershell
Start-Process -FilePath "D:\finderX\build\Release\FinderX.exe"
```

Verify in the app:

1. Right-click list background in a normal folder and choose `New Folder`; `New Folder` appears and is selected.
2. Repeat `New Folder`; `New Folder 2` appears and is selected.
3. Right-click list background and choose `New File`; `New Text Document.txt` appears and is selected.
4. Repeat `New File`; `New Text Document 2.txt` appears and is selected.
5. Right-click a folder and choose `Open in PowerShell`; PowerShell opens with that folder as working directory.
6. Right-click list background and choose `Open in PowerShell`; PowerShell opens with the current folder as working directory.
7. Create or navigate to a known-empty folder; it does not show a disclosure arrow after its children are known.
8. Click `This PC` in the sidebar; the main pane shows drive rows such as `C:\` and any other drives Windows reports.
9. Double-click a drive row; FinderX navigates into that drive root.
10. In `This PC`, right-click background and confirm file creation commands are not offered.

- [ ] **Step 4: Inspect git diff**

Run:

```powershell
git -c safe.directory=D:/finderX status --short
git -c safe.directory=D:/finderX diff --check
```

Expected:

- `git diff --check` prints no whitespace errors.
- `git status --short` only shows intended tracked changes or persistent unrelated untracked docs/templates already present before this task.

- [ ] **Step 5: Commit smoke notes only if source changed during smoke**

If no code changes are needed after smoke, do not create another commit. If a source fix is needed, run the targeted Debug and Release verification again before committing the fix.

---

## Final Verification Commands

Run these before reporting completion:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected:

- Debug build succeeds.
- Debug CTest passes.
- Release build succeeds.
- Release CTest passes.
- `git diff --check` has no output.
- Remaining untracked files, if present, match the persistent unrelated docs/template files already known in this repo.
