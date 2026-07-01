# FinderX Filesystem Loading Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the sample-only tree in FinderX with real Windows directory enumeration and lazy folder loading while preserving the Finder-style list interactions.

**Architecture:** Add a small filesystem layer that converts Win32 directory entries into existing `FileTree` nodes. Keep model behavior testable without Direct2D, and integrate loading through `MainWindow`/`FinderListView` with explicit expand events rather than hidden work inside paint.

**Tech Stack:** C++20 conservative subset, CMake, MSVC, Win32 filesystem APIs, existing Win32/Direct2D app.

---

## File Structure

Create:

- `src/fs/FileMetadata.h`: display-ready file metadata and loader result types.
- `src/fs/FileMetadata.cpp`: size, time, and kind formatting helpers.
- `src/fs/DirectoryLoader.h`: loader interface for enumerating a directory into `FileTree`.
- `src/fs/DirectoryLoader.cpp`: Win32 enumeration using `FindFirstFileW` / `FindNextFileW`.
- `src/fs/DirectoryLoaderTests.cpp`: console tests for formatting and a temporary directory enumeration.

Modify:

- `CMakeLists.txt`: add `finderx_fs` library, `finderx_fs_tests`, and link app to `finderx_fs`.
- `src/model/FileNode.h`: add `path` and `childrenLoaded` fields.
- `src/model/FileTree.h`, `src/model/FileTree.cpp`: support root construction from a real path and clearing/replacing children.
- `src/model/FileTreeTests.cpp`: cover new path and child replacement behavior.
- `src/ui/FinderListView.h`, `src/ui/FinderListView.cpp`: report folder expansion requests so the app can lazy-load before/after expansion.
- `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp`: initialize with the user profile/home directory and lazy-load folders when expanded.
- `src/ui/FinderChrome.cpp`: no required change in this milestone; path bar text remains static while the main list switches to real filesystem data.

---

### Task 1: Metadata Formatting Helpers

**Files:**
- Create: `src/fs/FileMetadata.h`
- Create: `src/fs/FileMetadata.cpp`
- Create: `src/fs/DirectoryLoaderTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add filesystem metadata header**

Create `src/fs/FileMetadata.h`:

```cpp
#pragma once

#include "model/FileNode.h"

#include <windows.h>

#include <string>

namespace finderx {

struct FileMetadata {
    std::wstring name;
    std::wstring path;
    FileKind kind = FileKind::File;
    std::wstring modified;
    std::wstring size;
    std::wstring kindText;
};

std::wstring formatFileSize(unsigned long long bytes, bool isDirectory);
std::wstring formatFileTime(const FILETIME& fileTime);
std::wstring kindTextForAttributes(DWORD attributes);
bool isSkippableDirectoryEntry(std::wstring_view name);

}
```

- [ ] **Step 2: Add metadata implementation**

Create `src/fs/FileMetadata.cpp`:

```cpp
#include "fs/FileMetadata.h"

#include <iomanip>
#include <sstream>

namespace finderx {

std::wstring formatFileSize(unsigned long long bytes, bool isDirectory) {
    if (isDirectory) {
        return L"--";
    }
    constexpr unsigned long long kib = 1024;
    constexpr unsigned long long mib = kib * 1024;
    constexpr unsigned long long gib = mib * 1024;
    std::wostringstream out;
    if (bytes >= gib) {
        out << std::fixed << std::setprecision(1) << static_cast<double>(bytes) / static_cast<double>(gib) << L" GB";
    } else if (bytes >= mib) {
        out << std::fixed << std::setprecision(1) << static_cast<double>(bytes) / static_cast<double>(mib) << L" MB";
    } else if (bytes >= kib) {
        out << static_cast<unsigned long long>((bytes + kib - 1) / kib) << L" KB";
    } else {
        out << bytes << L" bytes";
    }
    return out.str();
}

std::wstring formatFileTime(const FILETIME& fileTime) {
    FILETIME localFileTime{};
    SYSTEMTIME systemTime{};
    if (!FileTimeToLocalFileTime(&fileTime, &localFileTime) ||
        !FileTimeToSystemTime(&localFileTime, &systemTime)) {
        return L"";
    }
    std::wostringstream out;
    out << systemTime.wYear << L"/"
        << std::setw(2) << std::setfill(L'0') << systemTime.wMonth << L"/"
        << std::setw(2) << std::setfill(L'0') << systemTime.wDay << L" "
        << std::setw(2) << std::setfill(L'0') << systemTime.wHour << L":"
        << std::setw(2) << std::setfill(L'0') << systemTime.wMinute;
    return out.str();
}

std::wstring kindTextForAttributes(DWORD attributes) {
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? L"Folder" : L"File";
}

bool isSkippableDirectoryEntry(std::wstring_view name) {
    return name == L"." || name == L"..";
}

}
```

- [ ] **Step 3: Add formatting tests**

Create `src/fs/DirectoryLoaderTests.cpp`:

```cpp
#include "fs/FileMetadata.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

int main() {
    require(formatFileSize(0, true) == L"--", "directory size should be --");
    require(formatFileSize(42, false) == L"42 bytes", "byte size should be exact");
    require(formatFileSize(1536, false) == L"2 KB", "KB size should round up");
    require(formatFileSize(1024 * 1024, false) == L"1.0 MB", "MB size should use one decimal");
    require(kindTextForAttributes(FILE_ATTRIBUTE_DIRECTORY) == L"Folder", "directory kind text");
    require(kindTextForAttributes(FILE_ATTRIBUTE_ARCHIVE) == L"File", "file kind text");
    require(isSkippableDirectoryEntry(L"."), "dot should be skipped");
    require(isSkippableDirectoryEntry(L".."), "dot-dot should be skipped");
    require(!isSkippableDirectoryEntry(L"Documents"), "normal name should not be skipped");
    std::cout << "DirectoryLoaderTests passed\n";
    return 0;
}
```

- [ ] **Step 4: Update CMake**

Modify `CMakeLists.txt` to add:

```cmake
add_library(finderx_fs
    src/fs/FileMetadata.cpp
)

target_include_directories(finderx_fs PUBLIC src)
target_link_libraries(finderx_fs PUBLIC finderx_model)
finderx_enable_utf8(finderx_fs)
finderx_enable_windows_unicode(finderx_fs)

add_executable(finderx_fs_tests
    src/fs/DirectoryLoaderTests.cpp
)

target_link_libraries(finderx_fs_tests PRIVATE finderx_fs)
finderx_enable_utf8(finderx_fs_tests)
finderx_enable_windows_unicode(finderx_fs_tests)
```

Also add `finderx_fs` to `target_link_libraries(FinderX PRIVATE ...)`.

- [ ] **Step 5: Build and run tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_fs_tests
.\build\Debug\finderx_fs_tests.exe
```

Expected:

```text
DirectoryLoaderTests passed
```

- [ ] **Step 6: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/fs
git -c safe.directory=D:/finderX commit -m "feat: add filesystem metadata formatting"
```

---

### Task 2: FileTree Support For Real Paths

**Files:**
- Modify: `src/model/FileNode.h`
- Modify: `src/model/FileTree.h`
- Modify: `src/model/FileTree.cpp`
- Modify: `src/model/FileTreeTests.cpp`

- [ ] **Step 1: Extend file node state**

Modify `src/model/FileNode.h`:

```cpp
struct FileNode {
    NodeId id = kInvalidNodeId;
    NodeId parent = kInvalidNodeId;
    std::wstring name;
    std::wstring path;
    std::wstring modified;
    std::wstring size;
    std::wstring kindText;
    FileKind kind = FileKind::File;
    bool expanded = false;
    bool childrenLoaded = false;
    std::vector<NodeId> children;
};
```

- [ ] **Step 2: Extend FileTree API**

Modify `src/model/FileTree.h`:

```cpp
    explicit FileTree(std::wstring rootPath, std::wstring rootName);
    NodeId addNode(NodeId parent, std::wstring name, std::wstring path, FileKind kind,
                   std::wstring modified, std::wstring size, std::wstring kindText);
    void replaceChildren(NodeId parent, std::vector<FileNode> children);
    void setChildrenLoaded(NodeId id, bool loaded);
```

Keep the existing `addNode(NodeId parent, std::wstring name, FileKind kind, ...)` overload for sample/tests by forwarding with an empty path.

- [ ] **Step 3: Implement path-aware tree methods**

Modify `src/model/FileTree.cpp`:

```cpp
FileTree::FileTree(std::wstring rootPath, std::wstring rootName) {
    nodes_.push_back(FileNode{});
    rootId_ = addNode(kInvalidNodeId, std::move(rootName), std::move(rootPath), FileKind::Folder, L"", L"--", L"Folder");
    nodes_[rootId_].expanded = true;
}
```

Add the path-aware `addNode` overload before the existing one, and have the existing overload call it:

```cpp
NodeId FileTree::addNode(NodeId parent, std::wstring name, std::wstring path, FileKind kind,
                         std::wstring modified, std::wstring size, std::wstring kindText) {
    if (parent != kInvalidNodeId && node(parent).kind != FileKind::Folder) {
        throw std::invalid_argument("parent must be a folder");
    }
    const NodeId id = static_cast<NodeId>(nodes_.size());
    FileNode item;
    item.id = id;
    item.parent = parent;
    item.name = std::move(name);
    item.path = std::move(path);
    item.modified = std::move(modified);
    item.size = std::move(size);
    item.kindText = std::move(kindText);
    item.kind = kind;
    nodes_.push_back(std::move(item));
    if (parent != kInvalidNodeId) {
        node(parent).children.push_back(id);
    }
    return id;
}
```

Implement replacement by clearing current child ids and appending new nodes:

```cpp
void FileTree::replaceChildren(NodeId parent, std::vector<FileNode> children) {
    FileNode& parentNode = node(parent);
    if (parentNode.kind != FileKind::Folder) {
        throw std::invalid_argument("parent must be a folder");
    }
    parentNode.children.clear();
    for (FileNode child : children) {
        addNode(parent, std::move(child.name), std::move(child.path), child.kind,
                std::move(child.modified), std::move(child.size), std::move(child.kindText));
    }
    parentNode.childrenLoaded = true;
}

void FileTree::setChildrenLoaded(NodeId id, bool loaded) {
    node(id).childrenLoaded = loaded;
}
```

- [ ] **Step 4: Add model tests**

Modify `src/model/FileTreeTests.cpp` to add before the final success print:

```cpp
    FileTree realTree(L"C:\\Users\\Example", L"Example");
    require(realTree.node(realTree.rootId()).path == L"C:\\Users\\Example", "root path should be stored");
    std::vector<FileNode> children;
    FileNode child;
    child.name = L"Documents";
    child.path = L"C:\\Users\\Example\\Documents";
    child.kind = FileKind::Folder;
    child.size = L"--";
    child.kindText = L"Folder";
    children.push_back(std::move(child));
    realTree.replaceChildren(realTree.rootId(), std::move(children));
    require(realTree.node(realTree.rootId()).childrenLoaded, "replaceChildren should mark loaded");
    auto realRows = realTree.flatten();
    require(realRows.size() == 1, "real tree should flatten replaced child");
    require(realTree.node(realRows.front().nodeId).path == L"C:\\Users\\Example\\Documents", "child path should be stored");
```

- [ ] **Step 5: Build and run model tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_model_tests
.\build\Debug\finderx_model_tests.exe
```

Expected:

```text
FileTreeTests passed
```

- [ ] **Step 6: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/model
git -c safe.directory=D:/finderX commit -m "feat: add path-aware file tree state"
```

---

### Task 3: Directory Enumeration Loader

**Files:**
- Create: `src/fs/DirectoryLoader.h`
- Create: `src/fs/DirectoryLoader.cpp`
- Modify: `src/fs/DirectoryLoaderTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add loader interface**

Create `src/fs/DirectoryLoader.h`:

```cpp
#pragma once

#include "fs/FileMetadata.h"
#include "model/FileNode.h"

#include <string>
#include <vector>

namespace finderx {

class DirectoryLoader {
public:
    std::vector<FileNode> loadChildren(const std::wstring& directoryPath) const;
};

std::wstring defaultHomeDirectory();
std::wstring fileNameFromPath(const std::wstring& path);

}
```

- [ ] **Step 2: Add loader implementation**

Create `src/fs/DirectoryLoader.cpp`:

```cpp
#include "fs/DirectoryLoader.h"

#include "fs/FileMetadata.h"

#include <windows.h>

#include <algorithm>
#include <stdexcept>

namespace finderx {

namespace {

unsigned long long fileSizeFromFindData(const WIN32_FIND_DATAW& data) {
    ULARGE_INTEGER value{};
    value.HighPart = data.nFileSizeHigh;
    value.LowPart = data.nFileSizeLow;
    return value.QuadPart;
}

std::wstring joinSearchPattern(const std::wstring& directoryPath) {
    if (directoryPath.empty()) {
        return L"*";
    }
    const wchar_t last = directoryPath.back();
    if (last == L'\\' || last == L'/') {
        return directoryPath + L"*";
    }
    return directoryPath + L"\\*";
}

std::wstring joinPath(const std::wstring& directoryPath, const std::wstring& name) {
    if (directoryPath.empty()) {
        return name;
    }
    const wchar_t last = directoryPath.back();
    if (last == L'\\' || last == L'/') {
        return directoryPath + name;
    }
    return directoryPath + L"\\" + name;
}

}

std::vector<FileNode> DirectoryLoader::loadChildren(const std::wstring& directoryPath) const {
    std::vector<FileNode> nodes;
    WIN32_FIND_DATAW data{};
    const std::wstring pattern = joinSearchPattern(directoryPath);
    HANDLE find = FindFirstFileW(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return nodes;
    }

    do {
        const std::wstring name = data.cFileName;
        if (isSkippableDirectoryEntry(name)) {
            continue;
        }
        const bool isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        FileNode node;
        node.name = name;
        node.path = joinPath(directoryPath, name);
        node.kind = isDirectory ? FileKind::Folder : FileKind::File;
        node.modified = formatFileTime(data.ftLastWriteTime);
        node.size = formatFileSize(fileSizeFromFindData(data), isDirectory);
        node.kindText = kindTextForAttributes(data.dwFileAttributes);
        node.childrenLoaded = false;
        nodes.push_back(std::move(node));
    } while (FindNextFileW(find, &data));

    FindClose(find);
    std::stable_sort(nodes.begin(), nodes.end(), [](const FileNode& left, const FileNode& right) {
        if (left.kind != right.kind) {
            return left.kind == FileKind::Folder;
        }
        return _wcsicmp(left.name.c_str(), right.name.c_str()) < 0;
    });
    return nodes;
}

std::wstring defaultHomeDirectory() {
    wchar_t buffer[MAX_PATH]{};
    DWORD length = GetEnvironmentVariableW(L"USERPROFILE", buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::wstring(buffer, length);
    }
    length = GetEnvironmentVariableW(L"HOMEPATH", buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::wstring(buffer, length);
    }
    return L"C:\\";
}

std::wstring fileNameFromPath(const std::wstring& path) {
    const std::size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return path;
    }
    if (slash + 1 >= path.size()) {
        return path;
    }
    return path.substr(slash + 1);
}

}
```

- [ ] **Step 3: Update CMake library**

Modify `finderx_fs` in `CMakeLists.txt`:

```cmake
add_library(finderx_fs
    src/fs/FileMetadata.cpp
    src/fs/DirectoryLoader.cpp
)
```

- [ ] **Step 4: Add loader tests**

Modify `src/fs/DirectoryLoaderTests.cpp` to include:

```cpp
#include "fs/DirectoryLoader.h"

#include <filesystem>
#include <fstream>
```

Add before success print:

```cpp
    const auto tempRoot = std::filesystem::temp_directory_path() / "finderx_loader_test";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "FolderA");
    {
        std::ofstream file(tempRoot / "file-b.txt");
        file << "hello";
    }
    DirectoryLoader loader;
    const auto loaded = loader.loadChildren(tempRoot.wstring());
    require(loaded.size() == 2, "loader should enumerate temp directory children");
    require(loaded[0].kind == FileKind::Folder, "folders should sort before files");
    require(loaded[0].name == L"FolderA", "folder name should match");
    require(loaded[1].name == L"file-b.txt", "file name should match");
    require(!loaded[0].path.empty(), "loaded folder path should be stored");
    std::filesystem::remove_all(tempRoot);
```

- [ ] **Step 5: Build and run filesystem tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_fs_tests
.\build\Debug\finderx_fs_tests.exe
```

Expected:

```text
DirectoryLoaderTests passed
```

- [ ] **Step 6: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/fs
git -c safe.directory=D:/finderX commit -m "feat: enumerate directory children"
```

---

### Task 4: Integrate Real Root And Lazy Loading

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Expose expansion request result**

Modify `src/ui/FinderListView.h`:

```cpp
struct ListInteractionResult {
    bool changed = false;
    NodeId expandedFolder = kInvalidNodeId;
};
```

Change:

```cpp
bool onMouseDown(float x, float y, const D2D1_RECT_F& bounds);
```

to:

```cpp
ListInteractionResult onMouseDown(float x, float y, const D2D1_RECT_F& bounds);
```

Add:

```cpp
NodeId selectedNode() const;
```

- [ ] **Step 2: Return expansion requests from mouse/keyboard**

Modify `FinderListView::onMouseDown` so clicking a collapsed folder disclosure returns `expandedFolder = row.nodeId` after setting it expanded. If a folder is already expanded, collapse it and leave `expandedFolder` invalid.

Add:

```cpp
NodeId FinderListView::selectedNode() const {
    return selected_;
}
```

Change `onKeyDown` to return `NodeId`:

```cpp
NodeId onKeyDown(WPARAM key);
```

Return the selected folder id when Right expands a previously collapsed folder. Return `kInvalidNodeId` otherwise.

- [ ] **Step 3: Add loader state to main window**

Modify `src/ui/MainWindow.h`:

```cpp
#include "fs/DirectoryLoader.h"
```

Replace:

```cpp
FileTree tree_ = FileTree::sample();
```

with:

```cpp
DirectoryLoader directoryLoader_;
FileTree tree_;
```

Add private methods:

```cpp
void initializeFileTree();
void loadChildrenIfNeeded(NodeId folder);
```

- [ ] **Step 4: Initialize real root in MainWindow**

Modify `MainWindow::create` after HWND creation succeeds and before `ShowWindow`:

```cpp
initializeFileTree();
```

Implement:

```cpp
void MainWindow::initializeFileTree() {
    const std::wstring rootPath = defaultHomeDirectory();
    const std::wstring rootName = fileNameFromPath(rootPath).empty() ? rootPath : fileNameFromPath(rootPath);
    tree_ = FileTree(rootPath, rootName);
    loadChildrenIfNeeded(tree_.rootId());
}

void MainWindow::loadChildrenIfNeeded(NodeId folder) {
    if (folder == kInvalidNodeId) {
        return;
    }
    FileNode& node = tree_.node(folder);
    if (node.kind != FileKind::Folder || node.childrenLoaded) {
        return;
    }
    tree_.replaceChildren(folder, directoryLoader_.loadChildren(node.path));
}
```

- [ ] **Step 5: Wire lazy loading events**

Modify `WM_LBUTTONDOWN`:

```cpp
const ListInteractionResult result = listView_.onMouseDown(x, y, rects.list);
if (result.expandedFolder != kInvalidNodeId) {
    loadChildrenIfNeeded(result.expandedFolder);
}
if (result.changed || result.expandedFolder != kInvalidNodeId) {
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

Modify `WM_KEYDOWN`:

```cpp
const NodeId expanded = listView_.onKeyDown(wParam);
if (expanded != kInvalidNodeId) {
    loadChildrenIfNeeded(expanded);
}
InvalidateRect(hwnd_, nullptr, FALSE);
```

- [ ] **Step 6: Link app to filesystem library**

Ensure `FinderX` links `finderx_fs` in `CMakeLists.txt`:

```cmake
target_link_libraries(FinderX PRIVATE
    finderx_model
    finderx_fs
    d2d1
    dwrite
    windowscodecs
    ole32
    shell32
)
```

- [ ] **Step 7: Build and run tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_model_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_fs_tests
.\build\Debug\finderx_model_tests.exe
.\build\Debug\finderx_fs_tests.exe
```

Expected:

```text
FileTreeTests passed
DirectoryLoaderTests passed
```

- [ ] **Step 8: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src
git -c safe.directory=D:/finderX commit -m "feat: load real filesystem tree"
```

---

### Task 5: Verification And Release Build

**Files:**
- Modify only if verification exposes a defect.

- [ ] **Step 1: Run all tests**

Run:

```powershell
.\build\Debug\finderx_model_tests.exe
.\build\Debug\finderx_fs_tests.exe
```

Expected:

```text
FileTreeTests passed
DirectoryLoaderTests passed
```

- [ ] **Step 2: Build Debug and Release app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target FinderX
```

Expected:

- `D:\finderX\build\Debug\FinderX.exe`
- `D:\finderX\build\Release\FinderX.exe`

- [ ] **Step 3: Manual app check**

Run:

```powershell
.\build\Debug\FinderX.exe
```

Check:

- Initial list shows files/folders from the current Windows user profile directory.
- Folders sort before files.
- Expanding a folder loads its real children.
- Collapsing a folder hides its children.
- Mouse selection, wheel scrolling, and arrow keys still work.
- Access-denied or empty folders do not crash the app.

- [ ] **Step 4: Static checks**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected: no whitespace errors; only intentional untracked design artifacts may remain.

- [ ] **Step 5: Commit verification fixes if needed**

If fixes were needed:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src
git -c safe.directory=D:/finderX commit -m "fix: stabilize filesystem loading milestone"
```

If no fixes were needed, do not create an empty commit.
