# FinderX Multi-Select Batch Operations Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Finder/Explorer-style multi-selection and extend copy, cut, paste, and move-to-trash to operate on selected groups.

**Architecture:** `FinderListView` owns visible-row selection state and exposes selected node IDs. `MainWindow` converts selected nodes into file operation paths and keeps single-item actions single-target. `FileOperationState` and `ShellFileOperations` gain vector/span APIs while preserving existing single-item API compatibility.

**Tech Stack:** C++20, Win32, CMake, Direct2D UI, Windows Shell APIs, existing executable test style.

---

## File Structure

- Modify `src/ui/FinderListView.h`: add multi-select APIs, anchor/focus state, and mouse/key modifiers.
- Modify `src/ui/FinderListView.cpp`: implement multi-select state transitions and multi-row drawing.
- Create `src/ui/FinderListViewTests.cpp`: automated tests for selection mechanics using a sample `FileTree`.
- Modify `src/ui/FileOperationState.h`: replace single stored source path with ordered vector storage while keeping single-path helpers.
- Modify `src/ui/FileOperationState.cpp`: implement vector copy/cut state behavior.
- Modify `src/ui/FileOperationStateTests.cpp`: extend tests for multi-path state.
- Modify `src/shell/ShellFileOperations.h`: add span-based batch copy/move/trash overloads.
- Modify `src/shell/ShellFileOperations.cpp`: implement multi-source shell operations with absolute-path validation.
- Modify `src/ui/MainWindow.h`: add batch target/path helpers and replace single-path selection restore helpers with vector-aware helpers.
- Modify `src/ui/MainWindow.cpp`: wire context menu, keyboard shortcuts, and file operation handlers to multi-selection.
- Modify `CMakeLists.txt`: add `finderx_list_view_tests` target.

---

### Task 1: FinderListView Multi-Selection State

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`
- Create: `src/ui/FinderListViewTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update public list view API**

In `src/ui/FinderListView.h`, include `<span>` and add fields to `ListInteractionResult`:

```cpp
struct ListInteractionResult {
    bool changed = false;
    NodeId expandedFolder = kInvalidNodeId;
    NodeId activatedNode = kInvalidNodeId;
};
```

Keep the struct unchanged unless compile errors reveal duplicate declarations. Add these public methods after `selectedNode()`:

```cpp
    std::vector<NodeId> selectedNodes() const;
    bool hasSelection() const;
    bool isSelected(NodeId id) const;
    bool selectNodes(std::span<const NodeId> ids);
    bool selectAllVisible();
    bool clearSelection();
```

Change mouse/key signatures:

```cpp
    ListInteractionResult onMouseDown(float x, float y, const D2D1_RECT_F& bounds, bool controlDown = false, bool shiftDown = false);
    ListInteractionResult onKeyDown(WPARAM key, bool controlDown = false, bool shiftDown = false);
```

Add private helpers:

```cpp
    void pruneSelectionToVisible();
    void selectSingle(NodeId id);
    void setRangeSelection(NodeId anchor, NodeId target);
    bool selectionContains(NodeId id) const;
    int rowIndexForNode(NodeId id) const;
    NodeId firstSelectedVisibleNode() const;
```

Replace `selected_` meaning with focused row and add selection state:

```cpp
    NodeId selected_ = kInvalidNodeId;
    NodeId anchor_ = kInvalidNodeId;
    std::vector<NodeId> selectedNodes_;
```

- [ ] **Step 2: Implement selection invariants**

In `src/ui/FinderListView.cpp`, add helper functions near `selectedRowIndex()`:

```cpp
bool FinderListView::selectionContains(NodeId id) const {
    return std::find(selectedNodes_.begin(), selectedNodes_.end(), id) != selectedNodes_.end();
}

int FinderListView::rowIndexForNode(NodeId id) const {
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].nodeId == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

NodeId FinderListView::firstSelectedVisibleNode() const {
    for (const VisibleRow& row : rows_) {
        if (selectionContains(row.nodeId)) {
            return row.nodeId;
        }
    }
    return kInvalidNodeId;
}

void FinderListView::pruneSelectionToVisible() {
    selectedNodes_.erase(
        std::remove_if(selectedNodes_.begin(), selectedNodes_.end(), [this](NodeId id) {
            return rowIndexForNode(id) < 0;
        }),
        selectedNodes_.end());

    if (selected_ != kInvalidNodeId && rowIndexForNode(selected_) < 0) {
        selected_ = firstSelectedVisibleNode();
    }
    if (anchor_ != kInvalidNodeId && rowIndexForNode(anchor_) < 0) {
        anchor_ = selected_;
    }
}

void FinderListView::selectSingle(NodeId id) {
    selectedNodes_.clear();
    if (id != kInvalidNodeId && rowIndexForNode(id) >= 0) {
        selectedNodes_.push_back(id);
        selected_ = id;
        anchor_ = id;
    } else {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
    }
}

void FinderListView::setRangeSelection(NodeId anchor, NodeId target) {
    const int anchorIndex = rowIndexForNode(anchor);
    const int targetIndex = rowIndexForNode(target);
    if (anchorIndex < 0 || targetIndex < 0) {
        selectSingle(target);
        return;
    }

    selectedNodes_.clear();
    const int first = (std::min)(anchorIndex, targetIndex);
    const int last = (std::max)(anchorIndex, targetIndex);
    for (int i = first; i <= last; ++i) {
        selectedNodes_.push_back(rows_[static_cast<std::size_t>(i)].nodeId);
    }
    selected_ = target;
    anchor_ = anchor;
}
```

Update `ensureSelection()` so it rebuilds a valid single selection when no selected nodes remain:

```cpp
void FinderListView::ensureSelection() {
    if (rows_.empty()) {
        selected_ = kInvalidNodeId;
        anchor_ = kInvalidNodeId;
        selectedNodes_.clear();
        return;
    }

    pruneSelectionToVisible();
    if (!selectedNodes_.empty() && selected_ != kInvalidNodeId) {
        return;
    }

    const NodeId fallback = rows_.size() > 3 ? rows_[3].nodeId : rows_.front().nodeId;
    selectSingle(fallback);
}
```

- [ ] **Step 3: Implement public selection APIs**

Add:

```cpp
std::vector<NodeId> FinderListView::selectedNodes() const {
    return selectedNodes_;
}

bool FinderListView::hasSelection() const {
    return !selectedNodes_.empty();
}

bool FinderListView::isSelected(NodeId id) const {
    return selectionContains(id);
}

bool FinderListView::selectNodes(std::span<const NodeId> ids) {
    if (!tree_) {
        return false;
    }
    rebuildRows();

    std::vector<NodeId> next;
    for (NodeId id : ids) {
        if (id != kInvalidNodeId && rowIndexForNode(id) >= 0
            && std::find(next.begin(), next.end(), id) == next.end()) {
            next.push_back(id);
        }
    }
    const bool changed = next != selectedNodes_;
    selectedNodes_ = std::move(next);
    selected_ = selectedNodes_.empty() ? kInvalidNodeId : selectedNodes_.back();
    anchor_ = selected_;
    ensureSelectionVisible();
    return changed;
}

bool FinderListView::selectAllVisible() {
    if (!tree_) {
        return false;
    }
    rebuildRows();
    std::vector<NodeId> next;
    for (const VisibleRow& row : rows_) {
        next.push_back(row.nodeId);
    }
    const bool changed = next != selectedNodes_;
    selectedNodes_ = std::move(next);
    if (selected_ == kInvalidNodeId || rowIndexForNode(selected_) < 0) {
        selected_ = selectedNodes_.empty() ? kInvalidNodeId : selectedNodes_.front();
    }
    anchor_ = selected_;
    ensureSelectionVisible();
    return changed;
}

bool FinderListView::clearSelection() {
    const bool changed = !selectedNodes_.empty() || selected_ != kInvalidNodeId || anchor_ != kInvalidNodeId;
    selectedNodes_.clear();
    selected_ = kInvalidNodeId;
    anchor_ = kInvalidNodeId;
    return changed;
}
```

Update existing `selectNode(NodeId id)` to call `selectSingle(id)` after validating visibility and return whether `selectedNodes_` changed.

- [ ] **Step 4: Update drawing and mouse behavior**

In `draw`, replace:

```cpp
const bool selected = node.id == selected_;
```

with:

```cpp
const bool selected = selectionContains(node.id);
```

Update `onMouseDown` to use modifiers:

```cpp
ListInteractionResult FinderListView::onMouseDown(float x, float y, const D2D1_RECT_F& bounds, bool controlDown, bool shiftDown) {
    ListInteractionResult result;
    if (!tree_) {
        return result;
    }
    rebuildRows();
    ensureSelection();
    viewportHeight_ = (std::max)(0.0f, bounds.bottom - bounds.top);
    clampScroll();

    const int index = hitTestRow(x, y, bounds);
    if (index < 0) {
        return result;
    }

    const VisibleRow row = rows_[static_cast<std::size_t>(index)];
    FileNode& node = tree_->node(row.nodeId);
    if (node.kind == FileKind::Folder && hitTestDisclosure(x, bounds, row)) {
        if (!selectionContains(row.nodeId)) {
            selectSingle(row.nodeId);
            result.changed = true;
        }
        tree_->toggleExpanded(row.nodeId);
        if (node.expanded) {
            result.expandedFolder = row.nodeId;
        }
        rebuildRows();
        pruneSelectionToVisible();
        ensureSelection();
        clampScroll();
        result.changed = true;
        lastClickedNode_ = kInvalidNodeId;
        lastClickTime_ = 0;
        return result;
    }

    const std::vector<NodeId> before = selectedNodes_;
    if (shiftDown && anchor_ != kInvalidNodeId) {
        setRangeSelection(anchor_, row.nodeId);
    } else if (controlDown) {
        if (selectionContains(row.nodeId)) {
            selectedNodes_.erase(std::remove(selectedNodes_.begin(), selectedNodes_.end(), row.nodeId), selectedNodes_.end());
            selected_ = selectedNodes_.empty() ? kInvalidNodeId : selectedNodes_.back();
            anchor_ = selected_;
        } else {
            selectedNodes_.push_back(row.nodeId);
            selected_ = row.nodeId;
            anchor_ = row.nodeId;
        }
    } else {
        selectSingle(row.nodeId);
    }
    result.changed = before != selectedNodes_;

    const DWORD clickTime = GetTickCount();
    const bool doubleClick = lastClickedNode_ == row.nodeId && clickTime - lastClickTime_ <= GetDoubleClickTime();
    lastClickedNode_ = row.nodeId;
    lastClickTime_ = clickTime;
    if (doubleClick) {
        result.activatedNode = row.nodeId;
        result.changed = true;
    }
    return result;
}
```

- [ ] **Step 5: Update keyboard behavior**

Update `onKeyDown(WPARAM key, bool controlDown, bool shiftDown)`:

```cpp
case L'A':
    if (controlDown && selectAllVisible()) {
        result.changed = true;
    }
    break;
```

For Up/Down, compute `nextIndex`; if `shiftDown`, initialize `anchor_` to current selected row when invalid, call `setRangeSelection(anchor_, nextNode)`. Otherwise call `selectSingle(nextNode)`. Always call `ensureSelectionVisible()` and set `result.changed = true` when the focused row changes.

Keep Left, Right, and Return operating on `selected_`.

- [ ] **Step 6: Add list view tests**

Create `src/ui/FinderListViewTests.cpp`:

```cpp
#include "ui/FinderListView.h"

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

static D2D1_RECT_F listBounds() {
    return D2D1::RectF(0.0f, 0.0f, 900.0f, 600.0f);
}

int main() {
    FileTree tree = FileTree::sample();
    FinderListView view(&tree);
    const D2D1_RECT_F bounds = listBounds();
    const NodeId first = tree.flatten()[0].nodeId;
    const NodeId second = tree.flatten()[1].nodeId;
    const NodeId third = tree.flatten()[2].nodeId;

    view.selectNode(first);
    require(view.selectedNodes().size() == 1 && view.isSelected(first), "plain select should choose one node");

    view.onMouseDown(60.0f, 24.0f, bounds, true, false);
    require(view.isSelected(first) && view.isSelected(second), "ctrl click should add second row");

    view.onMouseDown(60.0f, 24.0f, bounds, true, false);
    require(!view.isSelected(second), "ctrl click selected row should toggle it off");

    view.selectNode(first);
    view.onMouseDown(60.0f, 44.0f, bounds, false, true);
    require(view.isSelected(first) && view.isSelected(second) && view.isSelected(third), "shift click should select range");

    require(view.selectAllVisible(), "ctrl-a helper should select visible rows");
    require(view.selectedNodes().size() == tree.flatten().size(), "select all should include visible rows");

    view.selectNode(first);
    view.onKeyDown(VK_DOWN, false, true);
    require(view.isSelected(first) && view.isSelected(second), "shift down should extend selection");
    view.onKeyDown(VK_DOWN, false, false);
    require(view.selectedNodes().size() == 1, "down without shift should collapse selection");

    std::cout << "FinderListViewTests passed\n";
    return 0;
}
```

- [ ] **Step 7: Wire list view tests into CMake**

Add after existing UI helper tests in `CMakeLists.txt`:

```cmake
add_executable(finderx_list_view_tests
    src/ui/FinderListViewTests.cpp
    src/ui/FinderListView.cpp
    src/ui/RenderContext.cpp
)

target_include_directories(finderx_list_view_tests PRIVATE src)
target_link_libraries(finderx_list_view_tests PRIVATE finderx_model d2d1 dwrite windowscodecs)
finderx_enable_utf8(finderx_list_view_tests)
finderx_enable_windows_unicode(finderx_list_view_tests)
add_test(NAME finderx_list_view_tests COMMAND finderx_list_view_tests)
```

- [ ] **Step 8: Run Task 1 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_list_view_tests
& "D:\finderX\build\Debug\finderx_list_view_tests.exe"
```

Expected: build succeeds and prints `FinderListViewTests passed`.

- [ ] **Step 9: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add list multi-selection"
```

---

### Task 2: Multi-Path FileOperationState

**Files:**
- Modify: `src/ui/FileOperationState.h`
- Modify: `src/ui/FileOperationState.cpp`
- Modify: `src/ui/FileOperationStateTests.cpp`

- [ ] **Step 1: Update state API**

In `src/ui/FileOperationState.h`, include `<vector>` and add vector methods while keeping single-path compatibility:

```cpp
    void setCopy(std::vector<std::wstring> paths);
    void setCut(std::vector<std::wstring> paths);
    const std::vector<std::wstring>& sourcePaths() const;
```

Replace private `std::wstring sourcePath_;` with:

```cpp
    std::vector<std::wstring> sourcePaths_;
```

Keep `const std::wstring& sourcePath() const;` for existing callers. It should return an empty static string when no source exists.

- [ ] **Step 2: Implement vector state**

In `src/ui/FileOperationState.cpp`, add:

```cpp
namespace {
std::vector<std::wstring> cleanPaths(std::vector<std::wstring> paths) {
    paths.erase(std::remove_if(paths.begin(), paths.end(), [](const std::wstring& path) {
        return path.empty();
    }), paths.end());
    return paths;
}
}
```

Implement vector setters:

```cpp
void FileOperationState::setCopy(std::vector<std::wstring> paths) {
    paths = cleanPaths(std::move(paths));
    if (paths.empty()) {
        clear();
        return;
    }
    kind_ = PendingFileOperationKind::Copy;
    sourcePaths_ = std::move(paths);
}

void FileOperationState::setCut(std::vector<std::wstring> paths) {
    paths = cleanPaths(std::move(paths));
    if (paths.empty()) {
        clear();
        return;
    }
    kind_ = PendingFileOperationKind::Cut;
    sourcePaths_ = std::move(paths);
}
```

Update single-path setters:

```cpp
void FileOperationState::setCopy(std::wstring path) {
    setCopy(std::vector<std::wstring>{std::move(path)});
}

void FileOperationState::setCut(std::wstring path) {
    setCut(std::vector<std::wstring>{std::move(path)});
}
```

Update `clear`, `hasPendingOperation`, and accessors:

```cpp
void FileOperationState::clear() {
    kind_ = PendingFileOperationKind::None;
    sourcePaths_.clear();
}

bool FileOperationState::hasPendingOperation() const {
    return kind_ != PendingFileOperationKind::None && !sourcePaths_.empty();
}

const std::vector<std::wstring>& FileOperationState::sourcePaths() const {
    return sourcePaths_;
}

const std::wstring& FileOperationState::sourcePath() const {
    static const std::wstring empty;
    return sourcePaths_.empty() ? empty : sourcePaths_.front();
}
```

- [ ] **Step 3: Extend tests**

In `src/ui/FileOperationStateTests.cpp`, after existing single-path assertions, add:

```cpp
    state.setCopy(std::vector<std::wstring>{L"C:\\Temp\\a.txt", L"C:\\Temp\\b.txt"});
    require(state.hasPendingOperation(), "multi copy should set pending operation");
    require(state.sourcePaths().size() == 2, "multi copy should store both paths");
    state.markPasteSucceeded();
    require(state.sourcePaths().size() == 2, "multi copy should remain after paste succeeds");

    state.setCut(std::vector<std::wstring>{L"C:\\Temp\\c.txt", L"", L"C:\\Temp\\d.txt"});
    require(state.kind() == PendingFileOperationKind::Cut, "multi cut kind should be stored");
    require(state.sourcePaths().size() == 2, "empty paths should be ignored");
    state.markPasteSucceeded();
    require(!state.hasPendingOperation(), "multi cut should clear after paste succeeds");

    state.setCopy(std::vector<std::wstring>{L""});
    require(!state.hasPendingOperation(), "all-empty copy path list should clear state");
```

- [ ] **Step 4: Run Task 2 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_file_operation_state_tests FinderX
& "D:\finderX\build\Debug\finderx_file_operation_state_tests.exe"
```

Expected: build succeeds and prints `FileOperationStateTests passed`.

- [ ] **Step 5: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/FileOperationState.h src/ui/FileOperationState.cpp src/ui/FileOperationStateTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: store multiple file operation paths"
```

---

### Task 3: Batch Shell File Operations

**Files:**
- Modify: `src/shell/ShellFileOperations.h`
- Modify: `src/shell/ShellFileOperations.cpp`

- [ ] **Step 1: Add span overload declarations**

In `src/shell/ShellFileOperations.h`, include `<span>` and `<vector>` is not required. Add:

```cpp
FileOperationResult moveToTrash(HWND owner, std::span<const std::wstring> paths);
FileOperationResult copyToDirectory(HWND owner, std::span<const std::wstring> sourcePaths, const std::wstring& destinationDirectory);
FileOperationResult moveToDirectory(HWND owner, std::span<const std::wstring> sourcePaths, const std::wstring& destinationDirectory);
```

Keep existing single-path overloads.

- [ ] **Step 2: Implement multi-string buffer**

In `src/shell/ShellFileOperations.cpp`, add:

```cpp
std::wstring doubleNullTerminated(std::span<const std::wstring> paths) {
    std::wstring value;
    for (const std::wstring& path : paths) {
        value.append(path);
        value.push_back(L'\0');
    }
    value.push_back(L'\0');
    return value;
}

bool allAbsolute(std::span<const std::wstring> paths) {
    return !paths.empty() && std::all_of(paths.begin(), paths.end(), [](const std::wstring& path) {
        return !path.empty() && std::filesystem::path(path).is_absolute();
    });
}
```

Update the internal shell runner to accept a span of sources:

```cpp
FileOperationResult runShellOperation(HWND owner, UINT operation, std::span<const std::wstring> fromPaths,
                                      const std::wstring& to, const std::wstring& failureMessage) {
    if (!allAbsolute(fromPaths)) {
        return failure(failureMessage);
    }

    SHFILEOPSTRUCTW fileOperation{};
    fileOperation.hwnd = owner;
    fileOperation.wFunc = operation;
    const std::wstring fromBuffer = doubleNullTerminated(fromPaths);
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
```

- [ ] **Step 3: Implement batch overloads and delegate single overloads**

Add:

```cpp
FileOperationResult moveToTrash(HWND owner, std::span<const std::wstring> paths) {
    return runShellOperation(owner, FO_DELETE, paths, L"", L"Cannot move items to trash");
}

FileOperationResult copyToDirectory(HWND owner, std::span<const std::wstring> sourcePaths, const std::wstring& destinationDirectory) {
    if (destinationDirectory.empty()) {
        return failure(L"Cannot paste here");
    }
    if (!std::filesystem::path(destinationDirectory).is_absolute()) {
        return failure(L"Cannot copy items");
    }
    return runShellOperation(owner, FO_COPY, sourcePaths, destinationDirectory, L"Cannot copy items");
}

FileOperationResult moveToDirectory(HWND owner, std::span<const std::wstring> sourcePaths, const std::wstring& destinationDirectory) {
    if (destinationDirectory.empty()) {
        return failure(L"Cannot paste here");
    }
    if (!std::filesystem::path(destinationDirectory).is_absolute()) {
        return failure(L"Cannot move items");
    }
    return runShellOperation(owner, FO_MOVE, sourcePaths, destinationDirectory, L"Cannot move items");
}
```

Update existing single overloads:

```cpp
FileOperationResult moveToTrash(HWND owner, const std::wstring& path) {
    return moveToTrash(owner, std::span<const std::wstring>(&path, 1));
}
```

For single copy/move, preserve previous single-item messages by checking the result and mapping batch messages:

```cpp
FileOperationResult copyToDirectory(HWND owner, const std::wstring& sourcePath, const std::wstring& destinationDirectory) {
    FileOperationResult result = copyToDirectory(owner, std::span<const std::wstring>(&sourcePath, 1), destinationDirectory);
    if (!result.success && result.message == L"Cannot copy items") {
        result.message = L"Cannot copy item";
    }
    return result;
}
```

Apply the same mapping for single move and trash: `Cannot move items` to `Cannot move item`, `Cannot move items to trash` to `Cannot move item to trash`.

- [ ] **Step 4: Run Task 3 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX
git -c safe.directory=D:/finderX diff --check -- src/shell/ShellFileOperations.h src/shell/ShellFileOperations.cpp
```

Expected: `FinderX.exe` builds and diff check exits 0.

- [ ] **Step 5: Commit**

```powershell
git -c safe.directory=D:/finderX add src/shell/ShellFileOperations.h src/shell/ShellFileOperations.cpp
git -c safe.directory=D:/finderX commit -m "feat: add batch shell file operations"
```

---

### Task 4: MainWindow Batch Integration

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add batch helpers to MainWindow.h**

Add private methods:

```cpp
    std::vector<NodeId> commandTargetNodes(bool includeSelection) const;
    std::vector<std::wstring> pathsForNodes(std::span<const NodeId> nodes) const;
    bool refreshCurrentDirectorySelecting(std::span<const std::wstring> selectedPaths);
```

Add explicit includes if they are not already present:

```cpp
#include <span>
#include <vector>
```

Keep existing single helpers until call sites are migrated.

- [ ] **Step 2: Pass modifiers to list view**

In `WM_LBUTTONDOWN`, compute:

```cpp
const bool controlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
const ListInteractionResult result = listView_.onMouseDown(x, y, rects.list, controlDown, shiftDown);
```

In `WM_KEYDOWN`, pass `controlDown` and `shiftDown` to `listView_.onKeyDown`.

Ensure Ctrl+A is handled by `FinderListView`, not intercepted by file operation handlers.

- [ ] **Step 3: Update right-click selection behavior**

In `showContextMenu`, after `contextNode_ = listView_.nodeAtPoint(...)`:

```cpp
const bool hasTarget = contextNode_ != kInvalidNodeId;
if (hasTarget && !listView_.isSelected(contextNode_)) {
    if (listView_.selectNode(contextNode_)) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}
```

This preserves group selection when right-clicking an already selected row and switches to one row when right-clicking an unselected row.

- [ ] **Step 4: Update context menu**

Compute:

```cpp
const std::vector<NodeId> targets = commandTargetNodes(true);
const bool singleTarget = targets.size() == 1;
```

Row menu:

```cpp
AppendMenuW(menu, MF_STRING, kCommandOpen, L"Open");
if (singleTarget) {
    AppendMenuW(menu, MF_STRING, kCommandRename, L"Rename");
}
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
```

Blank menu remains Paste when pending plus Refresh.

- [ ] **Step 5: Implement batch target helpers**

Add:

```cpp
std::vector<NodeId> MainWindow::commandTargetNodes(bool includeSelection) const {
    if (contextNode_ != kInvalidNodeId && contextNode_ < tree_.nodes().size()) {
        if (includeSelection && listView_.isSelected(contextNode_)) {
            return listView_.selectedNodes();
        }
        return {contextNode_};
    }
    return includeSelection ? listView_.selectedNodes() : std::vector<NodeId>{commandTargetNode()};
}

std::vector<std::wstring> MainWindow::pathsForNodes(std::span<const NodeId> nodes) const {
    std::vector<std::wstring> paths;
    for (NodeId nodeId : nodes) {
        if (nodeId != kInvalidNodeId && nodeId < tree_.nodes().size()) {
            paths.push_back(tree_.node(nodeId).path);
        }
    }
    return paths;
}
```

- [ ] **Step 6: Update refresh selection preservation**

Add vector overload:

```cpp
bool MainWindow::refreshCurrentDirectorySelecting(std::span<const std::wstring> selectedPaths) {
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

    std::vector<NodeId> restored;
    for (const std::wstring& path : selectedPaths) {
        for (const FileNode& node : tree_.nodes()) {
            if (node.path == path) {
                restored.push_back(node.id);
                break;
            }
        }
    }
    listView_.selectNodes(restored);
    contextNode_ = kInvalidNodeId;
    chromeState_.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}
```

Update single-path overload to call the span overload with a one-item vector when nonempty. Update `refreshCurrentDirectory()` to collect selected paths:

```cpp
const std::vector<NodeId> selected = listView_.selectedNodes();
return refreshCurrentDirectorySelecting(pathsForNodes(selected));
```

- [ ] **Step 7: Update operation handlers**

Rename:

```cpp
const std::vector<NodeId> targets = commandTargetNodes(true);
if (targets.size() != 1) { return; }
const NodeId target = targets.front();
```

Trash:

```cpp
const std::vector<NodeId> targets = commandTargetNodes(true);
const std::vector<std::wstring> paths = pathsForNodes(targets);
if (paths.empty()) { return; }
const shell::FileOperationResult result = shell::moveToTrash(hwnd_, paths);
if (!result.success) {
    setStatusText(result.message.empty() ? L"Cannot move items to trash" : result.message);
    refreshCurrentDirectorySelecting(std::span<const std::wstring>{});
    return;
}
refreshCurrentDirectorySelecting(std::span<const std::wstring>{});
```

Copy/Cut:

```cpp
const std::vector<NodeId> targets = commandTargetNodes(true);
const std::vector<std::wstring> paths = pathsForNodes(targets);
if (paths.empty()) { return; }
fileOperationState_.setCopy(paths);
```

Paste:

```cpp
if (fileOperationState_.kind() == ui::PendingFileOperationKind::Copy) {
    result = shell::copyToDirectory(hwnd_, fileOperationState_.sourcePaths(), destination);
} else {
    result = shell::moveToDirectory(hwnd_, fileOperationState_.sourcePaths(), destination);
}
```

Use fallback messages `Cannot copy items`, `Cannot move items`, or `Cannot paste here` based on state kind and result message.

- [ ] **Step 8: Keep single-target actions single-target**

Update `openContextNode`, `revealContextNode`, and `copyContextNodePath` to use `commandTargetNode()` and not batch targets. `Open` continues to open only the context/focused row.

- [ ] **Step 9: Run Task 4 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
git -c safe.directory=D:/finderX diff --check -- src/ui/MainWindow.h src/ui/MainWindow.cpp
```

Expected: build succeeds and all tests pass.

- [ ] **Step 10: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: wire batch operations into main window"
```

---

### Task 5: Final Verification And Manual Smoke

**Files:**
- No planned source edits.

- [ ] **Step 1: Build Debug and Release**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

Expected: both builds succeed.

- [ ] **Step 2: Run tests**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: model, filesystem, navigation, rename dialog, file operation state, and list view tests pass.

- [ ] **Step 3: Diff check**

```powershell
git -c safe.directory=D:/finderX diff --check
```

Expected: no output and exit code 0.

- [ ] **Step 4: Launch smoke**

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

- [ ] **Step 5: Manual UI checks with disposable files**

Create a temporary folder with at least four disposable files and verify:

- Ctrl+click selects and deselects multiple rows.
- Shift+click selects a visible range.
- Shift+Up/Shift+Down extends selection.
- Ctrl+A selects all visible rows.
- Right-click selected row keeps group selection.
- Right-click unselected row switches to that row.
- Rename appears only for a single target.
- Delete moves multiple selected disposable files to Recycle Bin.
- Ctrl+C then Ctrl+V copies multiple disposable files.
- Ctrl+X then Ctrl+V moves multiple disposable files and clears cut state.
- Open opens only one focused/context item.

- [ ] **Step 6: Commit verification fixes only if needed**

If manual smoke finds a defect, fix the smallest related file, rerun Steps 1-4, and commit:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FileOperationState.h src/ui/FileOperationState.cpp src/shell/ShellFileOperations.h src/shell/ShellFileOperations.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp CMakeLists.txt
git -c safe.directory=D:/finderX commit -m "fix: polish multi-select batch operations"
```

If no source changes are needed, do not create a verification commit.
