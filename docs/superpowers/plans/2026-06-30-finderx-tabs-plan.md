# FinderX Multi-Tab MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a visible Finder-like tab strip with `Ctrl+T`, `+` tab creation, mouse tab switching, and per-tab navigation/search/list state.

**Architecture:** Introduce a small UI tab state layer with stable tab allocation. Extend `FinderChrome` to draw and hit-test an independent tab strip above the toolbar. Refactor `MainWindow` to route existing navigation, refresh, search, file actions, and directory watcher behavior through the active tab.

**Tech Stack:** C++20, Win32 messages/timers, Direct2D/DirectWrite rendering, CMake/CTest.

---

## File Structure

- Create `src/ui/TabManager.h`: pure tab metadata/state manager for active index, creation, activation, titles, and per-tab scalar UI state used by tests.
- Create `src/ui/TabManagerTests.cpp`: TDD coverage for tab creation, activation, invalid activation, and per-tab isolation.
- Modify `CMakeLists.txt`: add `finderx_tab_manager_tests`.
- Modify `src/navigation/SidebarModel.h`: extend `ChromeState` with tab titles and active tab index after existing fields to preserve aggregate compatibility as much as possible.
- Modify `src/ui/FinderChrome.h/.cpp`: add tab strip layout, draw, and hit testing.
- Modify `src/ui/MainWindow.h/.cpp`: add `TabState`, tab collection, active tab helpers, `Ctrl+T`, `+` tab handling, tab activation, and active-tab-only watcher lifecycle.

Use full tool paths for verification:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
git -c safe.directory=D:/finderX diff --check
```

---

### Task 1: Pure Tab Manager

**Files:**
- Create: `src/ui/TabManager.h`
- Create: `src/ui/TabManagerTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing tab manager test**

Create `src/ui/TabManagerTests.cpp`:

```cpp
#include "ui/TabManager.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace finderx::ui;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    {
        TabManager tabs;
        tabs.initialize(L"C:\\Users\\Example");

        require(tabs.count() == 1, "initialize should create one tab");
        require(tabs.activeIndex() == 0, "initial tab should be active");
        require(tabs.active().path == L"C:\\Users\\Example", "initial tab path should be stored");
        require(tabs.active().title == L"Example", "initial tab title should use final path component");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\Users\\Example");
        tabs.active().searchText = L"doc";

        const std::size_t created = tabs.createTab(L"C:\\Users\\Example\\Downloads");
        require(created == 1, "created tab index should be returned");
        require(tabs.count() == 2, "createTab should append a tab");
        require(tabs.activeIndex() == 1, "new tab should become active");
        require(tabs.active().path == L"C:\\Users\\Example\\Downloads", "new tab path should be stored");
        require(tabs.active().title == L"Downloads", "new tab title should use final path component");
        require(tabs.active().searchText.empty(), "new tab search should start empty");

        tabs.activate(0);
        require(tabs.activeIndex() == 0, "activate should switch active index");
        require(tabs.active().searchText == L"doc", "search text should be isolated per tab");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\Users\\Example");
        tabs.createTab(L"C:\\Users\\Example\\Desktop");
        tabs.activate(99);
        require(tabs.activeIndex() == 1, "invalid activation should be ignored");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\");
        require(tabs.active().title == L"C:\\", "root-like path should fall back to full path");
    }

    std::cout << "TabManagerTests passed\n";
    return 0;
}
```

- [ ] **Step 2: Register the test target**

Modify `CMakeLists.txt` after `finderx_file_operation_state_tests`:

```cmake
add_executable(finderx_tab_manager_tests
    src/ui/TabManagerTests.cpp
)

target_include_directories(finderx_tab_manager_tests PRIVATE src)
finderx_enable_utf8(finderx_tab_manager_tests)
finderx_enable_windows_unicode(finderx_tab_manager_tests)
add_test(NAME finderx_tab_manager_tests COMMAND finderx_tab_manager_tests)
```

- [ ] **Step 3: Run test to verify it fails**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_tab_manager_tests
```

Expected: fail because `ui/TabManager.h` does not exist.

- [ ] **Step 4: Implement minimal pure tab manager**

Create `src/ui/TabManager.h`:

```cpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace finderx::ui {

struct TabInfo {
    std::wstring path;
    std::wstring title;
    std::wstring searchText;
    std::wstring statusText;
};

class TabManager {
public:
    void initialize(std::wstring path) {
        tabs_.clear();
        activeIndex_ = 0;
        tabs_.push_back(makeTab(std::move(path)));
    }

    std::size_t createTab(std::wstring path) {
        tabs_.push_back(makeTab(std::move(path)));
        activeIndex_ = tabs_.size() - 1;
        return activeIndex_;
    }

    void activate(std::size_t index) {
        if (index < tabs_.size()) {
            activeIndex_ = index;
        }
    }

    std::size_t count() const {
        return tabs_.size();
    }

    std::size_t activeIndex() const {
        return activeIndex_;
    }

    TabInfo& active() {
        return tabs_[activeIndex_];
    }

    const TabInfo& active() const {
        return tabs_[activeIndex_];
    }

    const std::vector<TabInfo>& tabs() const {
        return tabs_;
    }

private:
    static std::wstring titleFromPath(const std::wstring& path) {
        const std::size_t end = path.find_last_not_of(L"\\/");
        if (end == std::wstring::npos) {
            return path;
        }
        const std::size_t slash = path.find_last_of(L"\\/", end);
        if (slash == std::wstring::npos) {
            return path.substr(0, end + 1);
        }
        std::wstring title = path.substr(slash + 1, end - slash);
        return title.empty() ? path : title;
    }

    static TabInfo makeTab(std::wstring path) {
        TabInfo tab;
        tab.path = std::move(path);
        tab.title = titleFromPath(tab.path);
        return tab;
    }

    std::vector<TabInfo> tabs_;
    std::size_t activeIndex_ = 0;
};

} // namespace finderx::ui
```

- [ ] **Step 5: Verify test passes**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target finderx_tab_manager_tests
& "D:\finderX\build\Debug\finderx_tab_manager_tests.exe"
```

Expected: build succeeds and prints `TabManagerTests passed`.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/ui/TabManager.h src/ui/TabManagerTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add tab manager model"
```

---

### Task 2: Chrome Tab Strip Rendering And Hit Testing

**Files:**
- Modify: `src/navigation/SidebarModel.h`
- Modify: `src/ui/FinderChrome.h`
- Modify: `src/ui/FinderChrome.cpp`

- [ ] **Step 1: Extend chrome state and hit results**

Modify `src/navigation/SidebarModel.h` by appending fields after `searchFocused`:

```cpp
std::vector<std::wstring> tabTitles;
std::size_t activeTabIndex = 0;
```

Add `<cstddef>` include if needed.

Modify `src/ui/FinderChrome.h`:

```cpp
enum class ChromeHitKind {
    None,
    Back,
    Forward,
    SearchField,
    SidebarItem,
    Tab,
    NewTab
};

struct ChromeHitResult {
    ChromeHitKind kind = ChromeHitKind::None;
    std::size_t sidebarIndex = 0;
    std::size_t tabIndex = 0;
};
```

- [ ] **Step 2: Add tab strip layout constants and shift content down**

In `src/ui/FinderChrome.cpp`, add:

```cpp
constexpr float kTabStripHeight = 34.0f;
constexpr float kTabTop = 6.0f;
constexpr float kTabHeight = 28.0f;
constexpr float kTabLeftPadding = 184.0f;
constexpr float kTabWidth = 146.0f;
constexpr float kTabGap = 4.0f;
constexpr float kNewTabWidth = 30.0f;
```

Update `FinderChrome::layout` so toolbar starts below the tab strip:

```cpp
const float tabBottom = (std::min)(kTabStripHeight, bottom);
const float toolbarBottom = (std::min)(kTabStripHeight + kToolbarHeight, bottom);
const float headerBottom = (std::clamp)(kTabStripHeight + kHeaderBottom, toolbarBottom, bottom);

rects.sidebar = D2D1::RectF(0.0f, 0.0f, contentLeft, bottom);
rects.toolbar = D2D1::RectF(contentLeft, tabBottom, right, toolbarBottom);
rects.header = D2D1::RectF(contentLeft, toolbarBottom, right, headerBottom);
```

Keep `pathbar` and `list` logic based on the new `headerBottom`.

- [ ] **Step 3: Draw tab strip**

Add helpers near `searchFieldRect`:

```cpp
D2D1_RECT_F tabRect(std::size_t index, float right) {
    const float left = kTabLeftPadding + static_cast<float>(index) * (kTabWidth + kTabGap);
    return D2D1::RectF(left, kTabTop, (std::min)(left + kTabWidth, right - kNewTabWidth - 18.0f), kTabTop + kTabHeight);
}

D2D1_RECT_F newTabRect(std::size_t tabCount, float right) {
    const float left = kTabLeftPadding + static_cast<float>(tabCount) * (kTabWidth + kTabGap);
    return D2D1::RectF(left, kTabTop, (std::min)(left + kNewTabWidth, right - 8.0f), kTabTop + kTabHeight);
}
```

In `draw`, before toolbar drawing, render the tab strip:

```cpp
render.fillRect(D2D1::RectF(0.0f, 0.0f, rects.pathbar.right, kTabStripHeight), D2D1::ColorF(0.90f, 0.91f, 0.92f));
render.drawLine(D2D1::Point2F(0.0f, kTabStripHeight), D2D1::Point2F(rects.pathbar.right, kTabStripHeight), rgb(0.78f));

for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
    const D2D1_RECT_F rect = tabRect(index, rects.pathbar.right);
    if (rect.right - rect.left < 44.0f) {
        break;
    }
    const bool active = index == state.activeTabIndex;
    render.fillRoundedRect(
        D2D1::RoundedRect(rect, 7.0f, 7.0f),
        active ? D2D1::ColorF(0.98f, 0.98f, 0.98f) : D2D1::ColorF(0.82f, 0.83f, 0.84f));
    render.drawRoundedRect(
        D2D1::RoundedRect(rect, 7.0f, 7.0f),
        active ? D2D1::ColorF(0.76f, 0.76f, 0.76f) : D2D1::ColorF(0.78f, 0.78f, 0.78f));
    drawTextClipped(
        render,
        state.tabTitles[index],
        D2D1::RectF(rect.left + 12.0f, rect.top + 5.0f, rect.right - 10.0f, rect.bottom),
        rect,
        render.textFormat(),
        D2D1::ColorF(0.14f, 0.14f, 0.14f));
}

const D2D1_RECT_F plusRect = newTabRect(state.tabTitles.size(), rects.pathbar.right);
if (plusRect.right - plusRect.left >= 24.0f) {
    render.fillRoundedRect(D2D1::RoundedRect(plusRect, 7.0f, 7.0f), D2D1::ColorF(0.82f, 0.83f, 0.84f));
    drawTextClipped(render, L"+", D2D1::RectF(plusRect.left + 10.0f, plusRect.top + 3.0f, plusRect.right, plusRect.bottom), plusRect, render.headerTextFormat(), D2D1::ColorF(0.20f, 0.20f, 0.20f));
}
```

- [ ] **Step 4: Hit test tabs before toolbar controls**

At the start of `FinderChrome::hitTest`:

```cpp
for (std::size_t index = 0; index < state.tabTitles.size(); ++index) {
    const D2D1_RECT_F rect = tabRect(index, rects.pathbar.right);
    if (rect.right - rect.left < 44.0f) {
        break;
    }
    if (containsPoint(rect, x, y)) {
        return {ChromeHitKind::Tab, 0, index};
    }
}

const D2D1_RECT_F plusRect = newTabRect(state.tabTitles.size(), rects.pathbar.right);
if (plusRect.right - plusRect.left >= 24.0f && containsPoint(plusRect, x, y)) {
    return {ChromeHitKind::NewTab, 0, 0};
}
```

- [ ] **Step 5: Build and diff check**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
git -c safe.directory=D:/finderX diff --check -- src/navigation/SidebarModel.h src/ui/FinderChrome.h src/ui/FinderChrome.cpp
```

Expected: build succeeds and diff check has no whitespace errors.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add src/navigation/SidebarModel.h src/ui/FinderChrome.h src/ui/FinderChrome.cpp
git -c safe.directory=D:/finderX commit -m "feat: draw finder tabs"
```

---

### Task 3: MainWindow Active Tab State

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add stable TabState storage**

In `src/ui/MainWindow.h`, include `<memory>` and add:

```cpp
struct TabState {
    FileTree tree;
    FinderListView listView;
    NavigationHistory history;
    std::wstring searchText;
    bool searchFocused = false;
    std::wstring statusText;

    TabState(std::wstring rootPath, std::wstring rootName)
        : tree(std::move(rootPath), std::move(rootName)),
          listView(&tree) {}
};
```

Replace members:

```cpp
FileTree tree_;
FinderListView listView_{&tree_};
NavigationHistory history_;
std::wstring searchText_;
bool searchFocused_ = false;
```

with:

```cpp
std::vector<std::unique_ptr<TabState>> tabs_;
std::size_t activeTabIndex_ = 0;
```

Add helpers:

```cpp
TabState& activeTab();
const TabState& activeTab() const;
bool hasActiveTab() const;
bool createTabAtPath(const std::wstring& path);
void activateTab(std::size_t index);
std::vector<std::wstring> tabTitles() const;
```

- [ ] **Step 2: Implement active tab helpers**

In `MainWindow.cpp`:

```cpp
MainWindow::TabState& MainWindow::activeTab() {
    return *tabs_[activeTabIndex_];
}

const MainWindow::TabState& MainWindow::activeTab() const {
    return *tabs_[activeTabIndex_];
}

bool MainWindow::hasActiveTab() const {
    return activeTabIndex_ < tabs_.size() && tabs_[activeTabIndex_];
}

std::vector<std::wstring> MainWindow::tabTitles() const {
    std::vector<std::wstring> titles;
    titles.reserve(tabs_.size());
    for (const auto& tab : tabs_) {
        std::wstring title = fileNameFromPath(tab->history.currentPath());
        if (title.empty()) {
            title = tab->history.currentPath();
        }
        titles.push_back(std::move(title));
    }
    return titles;
}
```

- [ ] **Step 3: Convert initialization and navigation to active tab**

Refactor `navigateToDirectory` into active-tab mutation:

```cpp
bool MainWindow::navigateToDirectory(std::wstring path, HistoryMode mode) {
    if (path.empty()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    const std::wstring watchedPath = path;
    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(path);
    if (!result.ok()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    std::wstring rootName = fileNameFromPath(path);
    if (rootName.empty()) {
        rootName = path;
    }

    auto tab = std::make_unique<TabState>(path, rootName);
    tab->tree.replaceChildren(tab->tree.rootId(), std::move(result.children));

    switch (mode) {
    case HistoryMode::Initial:
    case HistoryMode::Replace:
        tab->history.setInitialPath(std::move(path));
        break;
    case HistoryMode::Push:
        tab->history = activeTab().history;
        tab->history.navigateTo(std::move(path));
        break;
    case HistoryMode::BackForward:
        tab->history = activeTab().history;
        break;
    }

    if (tabs_.empty()) {
        tabs_.push_back(std::move(tab));
        activeTabIndex_ = 0;
    } else {
        tabs_[activeTabIndex_] = std::move(tab);
    }

    activeTab().statusText.clear();
    startDirectoryWatcher(watchedPath);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}
```

Then update every `tree_`, `listView_`, `history_`, `searchText_`, and `searchFocused_` use in `MainWindow.cpp` to go through `activeTab()`.

Concrete replacements:

```cpp
tree_ -> activeTab().tree
listView_ -> activeTab().listView
history_ -> activeTab().history
searchText_ -> activeTab().searchText
searchFocused_ -> activeTab().searchFocused
chromeState_.statusText -> activeTab().statusText
```

For `setStatusText`, write:

```cpp
void MainWindow::setStatusText(std::wstring text) {
    activeTab().statusText = std::move(text);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

- [ ] **Step 4: Build**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

Expected: exit code 0. If the compiler reports a removed member such as `tree_`, `listView_`, `history_`, `searchText_`, or `searchFocused_`, update that exact use to the active-tab equivalent listed in Step 3 and rerun this same command before continuing.

- [ ] **Step 5: Run tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "refactor: store browser state per tab"
```

---

### Task 4: New Tab Creation And Tab Activation

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Implement createTabAtPath**

Add implementation:

```cpp
bool MainWindow::createTabAtPath(const std::wstring& path) {
    const std::wstring target = path.empty() ? homePath_ : path;
    DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(target);
    if (!result.ok()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    std::wstring rootName = fileNameFromPath(target);
    if (rootName.empty()) {
        rootName = target;
    }

    auto tab = std::make_unique<TabState>(target, std::move(rootName));
    tab->tree.replaceChildren(tab->tree.rootId(), std::move(result.children));
    tab->history.setInitialPath(target);

    tabs_.push_back(std::move(tab));
    activeTabIndex_ = tabs_.size() - 1;
    startDirectoryWatcher(target);
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}
```

- [ ] **Step 2: Implement activateTab**

```cpp
void MainWindow::activateTab(std::size_t index) {
    if (index >= tabs_.size() || index == activeTabIndex_) {
        return;
    }

    activeTab().searchFocused = false;
    activeTabIndex_ = index;
    startDirectoryWatcher(activeTab().history.currentPath());
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

- [ ] **Step 3: Wire Ctrl+T and chrome hits**

In `WM_KEYDOWN`, before search key handling:

```cpp
if (wParam == L'T' && controlDown) {
    createTabAtPath(hasActiveTab() ? activeTab().history.currentPath() : homePath_);
    return 0;
}
```

In the chrome hit switch:

```cpp
case ChromeHitKind::Tab:
    activateTab(chromeHit.tabIndex);
    return 0;
case ChromeHitKind::NewTab:
    createTabAtPath(hasActiveTab() ? activeTab().history.currentPath() : homePath_);
    return 0;
```

- [ ] **Step 4: Sync chrome tab state**

In `refreshChromeState`:

```cpp
chromeState_.tabTitles = tabTitles();
chromeState_.activeTabIndex = activeTabIndex_;
chromeState_.statusText = activeTab().statusText;
chromeState_.searchText = activeTab().searchText;
chromeState_.searchFocused = activeTab().searchFocused;
```

- [ ] **Step 5: Build and test**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
git -c safe.directory=D:/finderX diff --check -- src/ui/MainWindow.h src/ui/MainWindow.cpp
```

Expected: build succeeds, all tests pass, diff check clean.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add tab creation and switching"
```

---

### Task 5: Final Verification And Manual Smoke

**Files:**
- No planned source edits unless verification exposes a bug.

- [ ] **Step 1: Run Debug build**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
```

Expected: exit code 0.

- [ ] **Step 2: Run Debug tests**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
```

Expected: 100% tests pass.

- [ ] **Step 3: Run Release build**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

Expected: exit code 0. If `LNK1104` reports `FinderX.exe` cannot be opened, check for an already-running FinderX process, stop only that process, and rerun the command.

- [ ] **Step 4: Run Release tests**

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

Expected: 100% tests pass.

- [ ] **Step 5: Run diff check**

```powershell
git -c safe.directory=D:/finderX diff --check
```

Expected: no whitespace errors. Git line-ending warnings are acceptable if exit code is 0.

- [ ] **Step 6: Manual smoke**

Run:

```powershell
$p = Start-Process -FilePath "D:\finderX\build\Release\FinderX.exe" -PassThru
Start-Sleep -Seconds 2
Get-Process -Id $p.Id
```

Then manually verify:

- Press `Ctrl+T`; a second tab appears and becomes active.
- Click the first tab; it becomes active and shows its previous directory.
- Navigate in one tab via sidebar; the other tab remains on its previous path.
- Type a search query in one tab; switch tabs; search text is isolated.
- Create/delete a file externally in the active tab's directory; active tab refreshes.

Close the app or run:

```powershell
Stop-Process -Id $p.Id
```

- [ ] **Step 7: Final status**

Run:

```powershell
git -c safe.directory=D:/finderX status --short
git -c safe.directory=D:/finderX log --oneline -8
```

Expected: only known unrelated untracked files remain.
