# FinderX Current Directory Search Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the toolbar search field accept text and filter the current visible directory list by file/folder name.

**Architecture:** `MainWindow` owns search focus and query input, `FinderChrome` draws and hit-tests the search field, and `FinderListView` filters its flattened visible rows before selection, hit testing, drawing, and keyboard navigation.

**Tech Stack:** C++20, Win32 messages, Direct2D rendering, CMake, existing executable test style.

---

## File Structure

- Modify `src/ui/FinderListView.h`: add filter text API.
- Modify `src/ui/FinderListView.cpp`: apply case-insensitive name filtering in `rebuildRows`.
- Modify `src/ui/FinderListViewTests.cpp`: add filter behavior tests.
- Modify `src/navigation/SidebarModel.h`: extend `ChromeState` with search text/focus.
- Modify `src/ui/FinderChrome.h`: add `ChromeHitKind::SearchField`.
- Modify `src/ui/FinderChrome.cpp`: draw focused/query search field and hit-test it.
- Modify `src/ui/MainWindow.h`: add search state and helper methods.
- Modify `src/ui/MainWindow.cpp`: route click, Ctrl+F, WM_CHAR, Backspace, Escape, Enter, and navigation clearing.

---

### Task 1: FinderListView Filtering

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`
- Modify: `src/ui/FinderListViewTests.cpp`

- [ ] **Step 1: Add filter API to FinderListView**

In `src/ui/FinderListView.h`, add public methods after selection APIs:

```cpp
    void setFilterText(std::wstring text);
    const std::wstring& filterText() const;
    bool hasFilter() const;
```

Add private member:

```cpp
    std::wstring filterText_;
```

- [ ] **Step 2: Add case-insensitive matching helpers**

In `src/ui/FinderListView.cpp`, include `<cwctype>` if needed and add helpers in the anonymous namespace:

```cpp
std::wstring lowercase(std::wstring_view text) {
    std::wstring lowered;
    lowered.reserve(text.size());
    for (wchar_t ch : text) {
        lowered.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }
    return lowered;
}

bool nameMatchesFilter(const FileNode& node, const std::wstring& loweredFilter) {
    if (loweredFilter.empty()) {
        return true;
    }
    return lowercase(node.name).find(loweredFilter) != std::wstring::npos;
}
```

- [ ] **Step 3: Apply filter in `rebuildRows`**

Change `rebuildRows()` from assigning `tree_->flatten()` directly to:

```cpp
void FinderListView::rebuildRows() {
    if (!tree_) {
        rows_.clear();
        return;
    }

    std::vector<VisibleRow> flattened = tree_->flatten();
    if (filterText_.empty()) {
        rows_ = std::move(flattened);
        return;
    }

    const std::wstring loweredFilter = lowercase(filterText_);
    rows_.clear();
    for (const VisibleRow& row : flattened) {
        if (row.nodeId != kInvalidNodeId && row.nodeId < tree_->nodes().size()
            && nameMatchesFilter(tree_->node(row.nodeId), loweredFilter)) {
            rows_.push_back(row);
        }
    }
}
```

This ensures draw, hit testing, `selectedNodes`, `selectAllVisible`, and keyboard navigation all operate on filtered rows.

- [ ] **Step 4: Implement filter API**

Add:

```cpp
void FinderListView::setFilterText(std::wstring text) {
    if (filterText_ == text) {
        return;
    }
    filterText_ = std::move(text);
    rebuildRows();
    pruneSelectionToVisible();
    ensureSelection();
    ensureSelectionVisible();
}

const std::wstring& FinderListView::filterText() const {
    return filterText_;
}

bool FinderListView::hasFilter() const {
    return !filterText_.empty();
}
```

- [ ] **Step 5: Add filter tests**

Extend `src/ui/FinderListViewTests.cpp` before the final success print:

```cpp
    {
        FileTree tree = FileTree::sample();
        FinderListView view(&tree);
        const std::size_t unfilteredCount = tree.flatten().size();

        view.setFilterText(L"");
        require(!view.hasFilter(), "empty filter should not be active");
        require(view.selectAllVisible(), "select all should work without filter");
        require(view.selectedNodes().size() == unfilteredCount, "empty filter should expose all visible rows");

        view.setFilterText(L"README");
        require(view.hasFilter(), "non-empty filter should be active");
        require(view.selectAllVisible(), "select all should work with filter");
        require(view.selectedNodes().size() == 1, "README filter should match one visible row");

        view.setFilterText(L"definitely-not-present");
        require(view.selectedNodes().empty(), "unmatched filter should leave no visible selection");
        require(!view.selectAllVisible(), "select all should not change empty filtered selection");

        view.setFilterText(L"");
        require(!view.hasFilter(), "clearing filter should remove filter state");
        require(view.selectAllVisible(), "select all should work after clearing filter");
        require(view.selectedNodes().size() == unfilteredCount, "clearing filter should restore visible rows");
    }
```

If `README` does not match the current sample tree visible rows, use an actual visible sample row name from `FileTree::sample()`, but keep one case-insensitive assertion.

- [ ] **Step 6: Run Task 1 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_list_view_tests
& "D:\finderX\build\Debug\finderx_list_view_tests.exe"
git -c safe.directory=D:/finderX diff --check -- src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp
```

Expected: build succeeds, `FinderListViewTests passed`, diff check exits 0.

- [ ] **Step 7: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: filter finder list rows"
```

---

### Task 2: Chrome Search Field State And Hit Test

**Files:**
- Modify: `src/navigation/SidebarModel.h`
- Modify: `src/ui/FinderChrome.h`
- Modify: `src/ui/FinderChrome.cpp`

- [ ] **Step 1: Extend ChromeState**

In `src/navigation/SidebarModel.h`, add before `sidebarItems`:

```cpp
    std::wstring searchText;
    bool searchFocused = false;
```

Update any aggregate initialization compile errors by adding empty search values.

- [ ] **Step 2: Add search hit kind**

In `src/ui/FinderChrome.h`, add enum value:

```cpp
    SearchField
```

- [ ] **Step 3: Extract reusable search rectangle helper**

In `src/ui/FinderChrome.cpp`, add in the anonymous namespace:

```cpp
D2D1_RECT_F searchFieldRect(const D2D1_RECT_F& toolbar) {
    if (toolbar.right - toolbar.left < 240.0f) {
        return D2D1::RectF();
    }

    const float searchLeft = (std::max)(toolbar.left + 420.0f, toolbar.right - 202.0f);
    return clampRect(D2D1::RectF(searchLeft, 16.0f, toolbar.right - 14.0f, 44.0f), toolbar);
}
```

- [ ] **Step 4: Draw actual query and focus styling**

Replace the current search box draw block with:

```cpp
    const D2D1_RECT_F searchRect = searchFieldRect(rects.toolbar);
    if (searchRect.right - searchRect.left >= 80.0f) {
        render.fillRoundedRect(
            D2D1::RoundedRect(searchRect, 7.0f, 7.0f),
            D2D1::ColorF(1.0f, 1.0f, 1.0f));
        const D2D1_COLOR_F border = state.searchFocused
            ? D2D1::ColorF(0.00f, 0.42f, 0.88f)
            : rgb(0.88f);
        render.drawRoundedRect(D2D1::RoundedRect(searchRect, 7.0f, 7.0f), border, state.searchFocused ? 1.4f : 1.0f);

        const std::wstring_view text = state.searchText.empty()
            ? std::wstring_view(L"\u2315 Search")
            : std::wstring_view(state.searchText);
        const D2D1_COLOR_F textColor = state.searchText.empty()
            ? D2D1::ColorF(0.53f, 0.53f, 0.53f)
            : D2D1::ColorF(0.16f, 0.16f, 0.16f);
        drawTextClipped(
            render,
            text,
            D2D1::RectF(searchRect.left + 11.0f, searchRect.top + 4.0f, searchRect.right - 12.0f, searchRect.bottom),
            searchRect,
            render.textFormat(),
            textColor);
    }
```

- [ ] **Step 5: Hit-test search field**

In `FinderChrome::hitTest`, before sidebar loop or after nav buttons, add:

```cpp
    const D2D1_RECT_F searchRect = searchFieldRect(rects.toolbar);
    if (searchRect.right - searchRect.left >= 80.0f && containsPoint(searchRect, x, y)) {
        return {ChromeHitKind::SearchField, 0};
    }
```

- [ ] **Step 6: Run Task 2 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX
git -c safe.directory=D:/finderX diff --check -- src/navigation/SidebarModel.h src/ui/FinderChrome.h src/ui/FinderChrome.cpp
```

Expected: `FinderX.exe` builds, diff check exits 0.

- [ ] **Step 7: Commit**

```powershell
git -c safe.directory=D:/finderX add src/navigation/SidebarModel.h src/ui/FinderChrome.h src/ui/FinderChrome.cpp
git -c safe.directory=D:/finderX commit -m "feat: draw interactive search field"
```

---

### Task 3: MainWindow Search Input Routing

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add search state and helpers**

In `src/ui/MainWindow.h`, add private methods:

```cpp
    void focusSearch();
    void blurSearch();
    void clearSearch();
    void setSearchText(std::wstring text);
    bool handleSearchKeyDown(WPARAM key);
    bool handleSearchChar(WPARAM character);
```

Add members:

```cpp
    std::wstring searchText_;
    bool searchFocused_ = false;
```

- [ ] **Step 2: Add helper implementations**

In `src/ui/MainWindow.cpp`, add:

```cpp
void MainWindow::focusSearch() {
    searchFocused_ = true;
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::blurSearch() {
    if (!searchFocused_) {
        return;
    }
    searchFocused_ = false;
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::clearSearch() {
    setSearchText(L"");
}

void MainWindow::setSearchText(std::wstring text) {
    if (searchText_ == text) {
        return;
    }
    searchText_ = std::move(text);
    listView_.setFilterText(searchText_);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

- [ ] **Step 3: Implement search key handling**

Add:

```cpp
bool MainWindow::handleSearchKeyDown(WPARAM key) {
    if (!searchFocused_) {
        return false;
    }

    switch (key) {
    case VK_BACK:
        if (!searchText_.empty()) {
            std::wstring next = searchText_;
            next.pop_back();
            setSearchText(std::move(next));
        }
        return true;
    case VK_ESCAPE:
        if (!searchText_.empty()) {
            clearSearch();
        } else {
            blurSearch();
        }
        return true;
    case VK_RETURN:
        blurSearch();
        return true;
    default:
        return false;
    }
}

bool MainWindow::handleSearchChar(WPARAM character) {
    if (!searchFocused_) {
        return false;
    }

    if (character >= 32 && character != 127) {
        std::wstring next = searchText_;
        next.push_back(static_cast<wchar_t>(character));
        setSearchText(std::move(next));
        return true;
    }

    return true;
}
```

- [ ] **Step 4: Route mouse clicks**

In `WM_LBUTTONDOWN`, handle `ChromeHitKind::SearchField`:

```cpp
        case ChromeHitKind::SearchField:
            focusSearch();
            return 0;
```

Before calling `listView_.onMouseDown`, call `blurSearch();` so clicking list returns focus to list.

- [ ] **Step 5: Route keyboard messages**

Add `WM_CHAR` case:

```cpp
    case WM_CHAR:
        if (handleSearchChar(wParam)) {
            return 0;
        }
        return 0;
```

At the start of `WM_KEYDOWN`, after computing `controlDown` and `shiftDown`, handle Ctrl+F and focused search:

```cpp
        if (controlDown && wParam == L'F') {
            focusSearch();
            return 0;
        }

        if (handleSearchKeyDown(wParam)) {
            return 0;
        }
```

This should run before file operation shortcuts so normal list shortcuts do not fire while search is focused.

- [ ] **Step 6: Clear search on navigation**

In `navigateToDirectory`, after `listView_ = FinderListView(&tree_);`, clear search state without recursively calling invalidation-heavy helpers:

```cpp
    searchText_.clear();
    searchFocused_ = false;
    listView_.setFilterText(searchText_);
```

This covers sidebar, back/forward, and open-folder navigation because they all call `navigateToDirectory`.

- [ ] **Step 7: Preserve search through refresh**

In `refreshCurrentDirectorySelecting(std::span<const std::wstring> selectedPaths)`, after recreating `listView_`:

```cpp
    listView_.setFilterText(searchText_);
```

Then restore selected paths. This keeps normal refresh from clearing search.

- [ ] **Step 8: Update chrome state**

In `refreshChromeState`, assign:

```cpp
    chromeState_.searchText = searchText_;
    chromeState_.searchFocused = searchFocused_;
```

- [ ] **Step 9: Run Task 3 verification**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
git -c safe.directory=D:/finderX diff --check -- src/ui/MainWindow.h src/ui/MainWindow.cpp
```

Expected: build succeeds and all tests pass.

- [ ] **Step 10: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: route current directory search input"
```

---

### Task 4: Final Verification And Manual Smoke

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

Expected: all tests pass.

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

- [ ] **Step 5: Manual UI checks**

Verify in the running app:

- Click search field and type a visible file/folder fragment.
- Results filter immediately by name.
- Matching is case-insensitive.
- Backspace updates results.
- Escape clears non-empty search.
- Escape again exits search focus.
- `Ctrl+F` focuses search.
- `Ctrl+A` while search is not focused selects filtered visible rows.
- Right-click and file operations target filtered visible rows.
- Navigating to another folder clears search.

- [ ] **Step 6: Commit verification fixes only if needed**

If manual smoke finds a defect, fix the smallest related file, rerun Steps 1-4, then commit:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp src/navigation/SidebarModel.h src/ui/FinderChrome.h src/ui/FinderChrome.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "fix: polish current directory search"
```

If no source changes are needed, do not create a verification commit.
