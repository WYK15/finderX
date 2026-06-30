# FinderX Navigation Usability Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make FinderX usable as a real file browser with directory navigation, sidebar switching, history, file activation, and dynamic path display.

**Architecture:** Add small testable navigation models, then wire them into the existing Win32/Direct2D UI. Keep `FileTree` focused on visible tree state, `DirectoryLoader` focused on filesystem enumeration, and `MainWindow` focused on coordination.

**Tech Stack:** C++20 conservative subset, CMake, MSVC, Win32, Direct2D/DirectWrite, ShellExecuteW.

---

## File Structure

Create:

- `src/navigation/NavigationHistory.h`: path history model.
- `src/navigation/NavigationHistory.cpp`: back/forward behavior.
- `src/navigation/NavigationHistoryTests.cpp`: console tests for history behavior.
- `src/navigation/SidebarModel.h`: common folder shortcut model and chrome state types.
- `src/navigation/SidebarModel.cpp`: sidebar item construction and selection.

Modify:

- `CMakeLists.txt`: add `finderx_navigation`, `finderx_navigation_tests`, and link app to the navigation library.
- `src/ui/FinderChrome.h`: accept dynamic `ChromeState` and expose toolbar/sidebar hit testing.
- `src/ui/FinderChrome.cpp`: draw dynamic title, sidebar state, path bar, error status, and back/forward enabled state.
- `src/ui/FinderListView.h`: report activated node from double-click and Enter.
- `src/ui/FinderListView.cpp`: implement double-click activation and Enter activation without opening files.
- `src/ui/MainWindow.h`: own navigation/sidebar state and add navigation helpers.
- `src/ui/MainWindow.cpp`: route sidebar, toolbar, keyboard, double-click, directory loading, history, and ShellExecuteW.

---

### Task 1: Navigation History Model

**Files:**
- Create: `src/navigation/NavigationHistory.h`
- Create: `src/navigation/NavigationHistory.cpp`
- Create: `src/navigation/NavigationHistoryTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add navigation history header**

Create `src/navigation/NavigationHistory.h`:

```cpp
#pragma once

#include <string>
#include <vector>

namespace finderx {

class NavigationHistory {
public:
    void setInitialPath(std::wstring path);
    void navigateTo(std::wstring path);
    bool canGoBack() const;
    bool canGoForward() const;
    std::wstring backTarget() const;
    std::wstring forwardTarget() const;
    std::wstring goBack();
    std::wstring goForward();
    const std::wstring& currentPath() const;

private:
    std::wstring currentPath_;
    std::vector<std::wstring> backStack_;
    std::vector<std::wstring> forwardStack_;
};

}
```

- [ ] **Step 2: Add navigation history implementation**

Create `src/navigation/NavigationHistory.cpp`:

```cpp
#include "navigation/NavigationHistory.h"

#include <utility>

namespace finderx {

void NavigationHistory::setInitialPath(std::wstring path) {
    currentPath_ = std::move(path);
    backStack_.clear();
    forwardStack_.clear();
}

void NavigationHistory::navigateTo(std::wstring path) {
    if (path.empty() || path == currentPath_) {
        return;
    }

    if (!currentPath_.empty()) {
        backStack_.push_back(std::move(currentPath_));
    }
    currentPath_ = std::move(path);
    forwardStack_.clear();
}

bool NavigationHistory::canGoBack() const {
    return !backStack_.empty();
}

bool NavigationHistory::canGoForward() const {
    return !forwardStack_.empty();
}

std::wstring NavigationHistory::backTarget() const {
    return canGoBack() ? backStack_.back() : currentPath_;
}

std::wstring NavigationHistory::forwardTarget() const {
    return canGoForward() ? forwardStack_.back() : currentPath_;
}

std::wstring NavigationHistory::goBack() {
    if (!canGoBack()) {
        return currentPath_;
    }

    forwardStack_.push_back(std::move(currentPath_));
    currentPath_ = std::move(backStack_.back());
    backStack_.pop_back();
    return currentPath_;
}

std::wstring NavigationHistory::goForward() {
    if (!canGoForward()) {
        return currentPath_;
    }

    backStack_.push_back(std::move(currentPath_));
    currentPath_ = std::move(forwardStack_.back());
    forwardStack_.pop_back();
    return currentPath_;
}

const std::wstring& NavigationHistory::currentPath() const {
    return currentPath_;
}

}
```

- [ ] **Step 3: Add navigation history tests**

Create `src/navigation/NavigationHistoryTests.cpp`:

```cpp
#include "navigation/NavigationHistory.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

static void testNavigationHistory() {
    NavigationHistory history;
    history.setInitialPath(L"C:\\Users\\leo");
    require(history.currentPath() == L"C:\\Users\\leo", "initial current path");
    require(!history.canGoBack(), "initial back disabled");
    require(!history.canGoForward(), "initial forward disabled");

    history.navigateTo(L"C:\\Users\\leo\\Documents");
    require(history.currentPath() == L"C:\\Users\\leo\\Documents", "navigate current path");
    require(history.canGoBack(), "back enabled after navigation");
    require(!history.canGoForward(), "forward disabled after navigation");

    require(history.goBack() == L"C:\\Users\\leo", "go back returns previous path");
    require(history.currentPath() == L"C:\\Users\\leo", "go back updates current path");
    require(!history.canGoBack(), "back disabled at history start");
    require(history.canGoForward(), "forward enabled after back");
    require(history.forwardTarget() == L"C:\\Users\\leo\\Documents", "forward target after back");

    require(history.goForward() == L"C:\\Users\\leo\\Documents", "go forward returns next path");
    require(history.currentPath() == L"C:\\Users\\leo\\Documents", "go forward updates current path");

    history.goBack();
    history.navigateTo(L"C:\\Users\\leo\\Downloads");
    require(history.currentPath() == L"C:\\Users\\leo\\Downloads", "new navigation after back");
    require(!history.canGoForward(), "new navigation clears forward stack");

    history.navigateTo(L"C:\\Users\\leo\\Downloads");
    require(history.currentPath() == L"C:\\Users\\leo\\Downloads", "same path navigation is ignored");
    require(history.backTarget() == L"C:\\Users\\leo", "back target after same path ignored");
    require(history.goBack() == L"C:\\Users\\leo", "same path did not duplicate history");
}

int main() {
    testNavigationHistory();
    std::cout << "NavigationHistoryTests passed\n";
    return 0;
}
```

- [ ] **Step 4: Update CMake**

Modify `CMakeLists.txt`:

```cmake
add_library(finderx_navigation
    src/navigation/NavigationHistory.cpp
)

target_include_directories(finderx_navigation PUBLIC src)
target_link_libraries(finderx_navigation PUBLIC finderx_fs)
finderx_enable_utf8(finderx_navigation)
finderx_enable_windows_unicode(finderx_navigation)

add_executable(finderx_navigation_tests
    src/navigation/NavigationHistoryTests.cpp
)

target_link_libraries(finderx_navigation_tests PRIVATE finderx_navigation)
finderx_enable_utf8(finderx_navigation_tests)
finderx_enable_windows_unicode(finderx_navigation_tests)
add_test(NAME finderx_navigation_tests COMMAND finderx_navigation_tests)
```

Also add `finderx_navigation` to `target_link_libraries(FinderX PRIVATE ...)`.

- [ ] **Step 5: Build and run tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_navigation_tests
.\build\Debug\finderx_navigation_tests.exe
```

Expected:

```text
NavigationHistoryTests passed
```

- [ ] **Step 6: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/navigation
git -c safe.directory=D:/finderX commit -m "feat: add navigation history model"
```

---

### Task 2: Sidebar Model And Chrome State Types

**Files:**
- Create: `src/navigation/SidebarModel.h`
- Create: `src/navigation/SidebarModel.cpp`
- Modify: `src/navigation/NavigationHistoryTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add sidebar model header**

Create `src/navigation/SidebarModel.h`:

```cpp
#pragma once

#include <string>
#include <vector>

namespace finderx {

struct SidebarItem {
    std::wstring label;
    std::wstring path;
    bool available = false;
    bool selected = false;
};

struct ChromeState {
    std::wstring title;
    std::wstring pathText;
    std::wstring statusText;
    bool canGoBack = false;
    bool canGoForward = false;
    std::vector<SidebarItem> sidebarItems;
};

class SidebarModel {
public:
    void refresh(const std::wstring& homePath, const std::wstring& currentPath);
    const std::vector<SidebarItem>& items() const;
    void setAvailabilityForTests(std::wstring label, bool available);

private:
    std::vector<SidebarItem> items_;
};

std::wstring joinPathForNavigation(const std::wstring& base, const std::wstring& child);
std::wstring userProgramsDirectory();

}
```

- [ ] **Step 2: Add sidebar model implementation**

Create `src/navigation/SidebarModel.cpp`:

```cpp
#include "navigation/SidebarModel.h"

#include <windows.h>

#include <algorithm>
#include <utility>

namespace finderx {
namespace {

bool directoryExists(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }

    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::wstring environmentVariable(const wchar_t* name) {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(name, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return L"";
    }
    return std::wstring(buffer, length);
}

bool pathsEqual(const std::wstring& left, const std::wstring& right) {
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

} // namespace

std::wstring joinPathForNavigation(const std::wstring& base, const std::wstring& child) {
    if (base.empty()) {
        return child;
    }

    const wchar_t last = base.back();
    if (last == L'\\' || last == L'/') {
        return base + child;
    }
    return base + L"\\" + child;
}

std::wstring userProgramsDirectory() {
    const std::wstring appData = environmentVariable(L"APPDATA");
    if (appData.empty()) {
        return L"";
    }
    return joinPathForNavigation(appData, L"Microsoft\\Windows\\Start Menu\\Programs");
}

void SidebarModel::refresh(const std::wstring& homePath, const std::wstring& currentPath) {
    std::vector<SidebarItem> next;
    next.push_back(SidebarItem{L"Home", homePath});
    next.push_back(SidebarItem{L"Desktop", joinPathForNavigation(homePath, L"Desktop")});
    next.push_back(SidebarItem{L"Documents", joinPathForNavigation(homePath, L"Documents")});
    next.push_back(SidebarItem{L"Downloads", joinPathForNavigation(homePath, L"Downloads")});
    next.push_back(SidebarItem{L"Applications", userProgramsDirectory()});

    for (SidebarItem& item : next) {
        item.available = directoryExists(item.path);
        item.selected = item.available && pathsEqual(item.path, currentPath);
    }

    items_ = std::move(next);
}

const std::vector<SidebarItem>& SidebarModel::items() const {
    return items_;
}

void SidebarModel::setAvailabilityForTests(std::wstring label, bool available) {
    for (SidebarItem& item : items_) {
        if (item.label == label) {
            item.available = available;
        }
    }
}

}
```

- [ ] **Step 3: Extend navigation tests with sidebar cases**

Modify `src/navigation/NavigationHistoryTests.cpp` to add the sidebar include:

```cpp
#include "navigation/SidebarModel.h"
```

Add this function before `main()`:

```cpp
static const SidebarItem* findItem(const std::vector<SidebarItem>& items, const std::wstring& label) {
    for (const SidebarItem& item : items) {
        if (item.label == label) {
            return &item;
        }
    }
    return nullptr;
}

static void testSidebarModel() {
    SidebarModel sidebar;
    sidebar.refresh(L"C:\\Users\\leo", L"C:\\Users\\leo\\Documents");
    const auto& items = sidebar.items();
    require(items.size() == 5, "sidebar item count");

    const SidebarItem* home = findItem(items, L"Home");
    const SidebarItem* desktop = findItem(items, L"Desktop");
    const SidebarItem* documents = findItem(items, L"Documents");
    const SidebarItem* downloads = findItem(items, L"Downloads");
    require(home && home->path == L"C:\\Users\\leo", "home path");
    require(desktop && desktop->path == L"C:\\Users\\leo\\Desktop", "desktop path");
    require(documents && documents->path == L"C:\\Users\\leo\\Documents", "documents path");
    require(downloads && downloads->path == L"C:\\Users\\leo\\Downloads", "downloads path");

    sidebar.setAvailabilityForTests(L"Documents", true);
    sidebar.setAvailabilityForTests(L"Downloads", false);
    require(findItem(sidebar.items(), L"Documents")->available, "test availability override true");
    require(!findItem(sidebar.items(), L"Downloads")->available, "test availability override false");
}
```

Update `main()`:

```cpp
int main() {
    testNavigationHistory();
    testSidebarModel();
    std::cout << "NavigationHistoryTests passed\n";
    return 0;
}
```

- [ ] **Step 4: Update CMake navigation library**

Modify `finderx_navigation` in `CMakeLists.txt`:

```cmake
add_library(finderx_navigation
    src/navigation/NavigationHistory.cpp
    src/navigation/SidebarModel.cpp
)
```

- [ ] **Step 5: Build and run navigation tests**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_navigation_tests
.\build\Debug\finderx_navigation_tests.exe
```

Expected:

```text
NavigationHistoryTests passed
```

- [ ] **Step 6: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src/navigation
git -c safe.directory=D:/finderX commit -m "feat: add sidebar navigation model"
```

---

### Task 3: Dynamic Chrome Rendering And Hit Testing

**Files:**
- Modify: `src/ui/FinderChrome.h`
- Modify: `src/ui/FinderChrome.cpp`
- Modify: `CMakeLists.txt` only if include dependencies require it

- [ ] **Step 1: Add chrome hit result API**

Modify `src/ui/FinderChrome.h`:

```cpp
#pragma once

#include "navigation/SidebarModel.h"
#include "ui/RenderContext.h"

#include <d2d1.h>

#include <cstddef>

namespace finderx {

struct LayoutRects {
    D2D1_RECT_F sidebar{};
    D2D1_RECT_F toolbar{};
    D2D1_RECT_F header{};
    D2D1_RECT_F list{};
    D2D1_RECT_F pathbar{};
};

enum class ChromeHitKind {
    None,
    Back,
    Forward,
    SidebarItem
};

struct ChromeHitResult {
    ChromeHitKind kind = ChromeHitKind::None;
    std::size_t sidebarIndex = 0;
};

class FinderChrome {
public:
    LayoutRects layout(float width, float height) const;
    void draw(RenderContext& render, const LayoutRects& rects, const ChromeState& state);
    ChromeHitResult hitTest(float x, float y, const LayoutRects& rects, const ChromeState& state) const;
};

}
```

- [ ] **Step 2: Update chrome draw signature**

Modify `FinderChrome::draw` signature in `src/ui/FinderChrome.cpp`:

```cpp
void FinderChrome::draw(RenderContext& render, const LayoutRects& rects, const ChromeState& state) {
```

Replace static sidebar text with a loop:

```cpp
    drawTextClipped(
        render,
        L"FAVORITES",
        D2D1::RectF(18.0f, 70.0f, 150.0f, 91.0f),
        rects.sidebar,
        render.headerTextFormat(),
        D2D1::ColorF(0.50f, 0.50f, 0.50f));

    float sidebarY = 94.0f;
    for (const SidebarItem& item : state.sidebarItems) {
        const D2D1_RECT_F rowRect = D2D1::RectF(24.0f, sidebarY - 2.0f, 164.0f, sidebarY + 22.0f);
        if (item.selected) {
            render.fillRoundedRect(
                D2D1::RoundedRect(clampRect(rowRect, rects.sidebar), 6.0f, 6.0f),
                D2D1::ColorF(0.80f, 0.82f, 0.84f));
        }
        const D2D1_COLOR_F color = item.available
            ? D2D1::ColorF(0.10f, 0.10f, 0.10f)
            : D2D1::ColorF(0.58f, 0.58f, 0.58f);
        drawTextClipped(
            render,
            item.label,
            D2D1::RectF(34.0f, sidebarY, 160.0f, sidebarY + 20.0f),
            rects.sidebar,
            render.textFormat(),
            color);
        sidebarY += 28.0f;
    }
```

Replace static toolbar title `L"home"` with:

```cpp
    drawTextClipped(
        render,
        state.title.empty() ? L"FinderX" : state.title,
        D2D1::RectF(rects.toolbar.left + 106.0f, 16.0f, rects.toolbar.left + 300.0f, 45.0f),
        rects.toolbar,
        render.headerTextFormat(),
        D2D1::ColorF(0.18f, 0.18f, 0.18f));
```

Replace static back/forward text with separate colors:

```cpp
    drawTextClipped(
        render,
        L"\u2039",
        D2D1::RectF(rects.toolbar.left + 22.0f, 15.0f, rects.toolbar.left + 45.0f, 43.0f),
        rects.toolbar,
        render.textFormat(),
        state.canGoBack ? D2D1::ColorF(0.22f, 0.22f, 0.22f) : D2D1::ColorF(0.68f, 0.68f, 0.68f));
    drawTextClipped(
        render,
        L"\u203a",
        D2D1::RectF(rects.toolbar.left + 58.0f, 15.0f, rects.toolbar.left + 82.0f, 43.0f),
        rects.toolbar,
        render.textFormat(),
        state.canGoForward ? D2D1::ColorF(0.22f, 0.22f, 0.22f) : D2D1::ColorF(0.68f, 0.68f, 0.68f));
```

Replace static pathbar text with:

```cpp
    const std::wstring pathText = state.statusText.empty() ? state.pathText : state.statusText;
    drawTextClipped(
        render,
        pathText,
        D2D1::RectF(rects.pathbar.left + 14.0f, rects.pathbar.top + 5.0f, rects.pathbar.right - 12.0f, rects.pathbar.bottom),
        window,
        render.textFormat(),
        state.statusText.empty() ? D2D1::ColorF(0.34f, 0.34f, 0.34f) : D2D1::ColorF(0.74f, 0.18f, 0.18f));
```

- [ ] **Step 3: Add chrome hit testing**

Add to `src/ui/FinderChrome.cpp`:

```cpp
ChromeHitResult FinderChrome::hitTest(float x, float y, const LayoutRects& rects, const ChromeState& state) const {
    if (y >= rects.toolbar.top && y <= rects.toolbar.bottom) {
        const D2D1_RECT_F backRect = D2D1::RectF(rects.toolbar.left + 18.0f, 8.0f, rects.toolbar.left + 48.0f, 48.0f);
        const D2D1_RECT_F forwardRect = D2D1::RectF(rects.toolbar.left + 52.0f, 8.0f, rects.toolbar.left + 86.0f, 48.0f);
        if (x >= backRect.left && x <= backRect.right && y >= backRect.top && y <= backRect.bottom && state.canGoBack) {
            return ChromeHitResult{ChromeHitKind::Back, 0};
        }
        if (x >= forwardRect.left && x <= forwardRect.right && y >= forwardRect.top && y <= forwardRect.bottom && state.canGoForward) {
            return ChromeHitResult{ChromeHitKind::Forward, 0};
        }
    }

    if (x >= rects.sidebar.left && x <= rects.sidebar.right && y >= rects.sidebar.top && y <= rects.sidebar.bottom) {
        float sidebarY = 94.0f;
        for (std::size_t index = 0; index < state.sidebarItems.size(); ++index) {
            const D2D1_RECT_F rowRect = D2D1::RectF(24.0f, sidebarY - 2.0f, 164.0f, sidebarY + 22.0f);
            if (x >= rowRect.left && x <= rowRect.right && y >= rowRect.top && y <= rowRect.bottom
                && state.sidebarItems[index].available) {
                return ChromeHitResult{ChromeHitKind::SidebarItem, index};
            }
            sidebarY += 28.0f;
        }
    }

    return ChromeHitResult{};
}
```

- [ ] **Step 4: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: `D:\finderX\build\Debug\FinderX.exe` builds.

- [ ] **Step 5: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderChrome.h src/ui/FinderChrome.cpp CMakeLists.txt
git -c safe.directory=D:/finderX commit -m "feat: render dynamic finder chrome"
```

---

### Task 4: List Activation Events

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`

- [ ] **Step 1: Extend list interaction result**

Modify `ListInteractionResult` in `src/ui/FinderListView.h`:

```cpp
struct ListInteractionResult {
    bool changed = false;
    NodeId expandedFolder = kInvalidNodeId;
    NodeId activatedNode = kInvalidNodeId;
};
```

Add private members:

```cpp
    NodeId lastClickedNode_ = kInvalidNodeId;
    DWORD lastClickTime_ = 0;
```

- [ ] **Step 2: Add double-click detection**

In `FinderListView::onMouseDown`, after selecting the row and before disclosure handling, add:

```cpp
    const DWORD clickTime = GetTickCount();
    const bool doubleClick = lastClickedNode_ == row.nodeId
        && clickTime - lastClickTime_ <= static_cast<DWORD>(GetDoubleClickTime());
    lastClickedNode_ = row.nodeId;
    lastClickTime_ = clickTime;
```

After disclosure handling, add:

```cpp
    if (!hitTestDisclosure(x, bounds, row) && doubleClick) {
        result.activatedNode = row.nodeId;
        result.changed = true;
    }
```

Ensure disclosure clicks do not activate the row.

- [ ] **Step 3: Return activation from Enter**

Change `FinderListView::onKeyDown(WPARAM key)` to return a result instead of only `NodeId`:

```cpp
ListInteractionResult onKeyDown(WPARAM key);
```

Update implementation:

```cpp
ListInteractionResult FinderListView::onKeyDown(WPARAM key) {
    ListInteractionResult result;
    if (!tree_) {
        return result;
    }
    ...
    case VK_RETURN:
        result.activatedNode = selected_;
        result.changed = true;
        break;
    ...
    case VK_RIGHT:
        ...
            result.expandedFolder = selected_;
            result.changed = true;
        ...
```

Set `result.changed = true` for Up, Down, Left, and Right when they actually change selection or expansion.

- [ ] **Step 4: Adjust callers only enough to compile**

Temporarily update `MainWindow.cpp` keyboard handling to use the new result and still call `loadChildrenIfNeeded(result.expandedFolder)`. Full activation routing is Task 5.

- [ ] **Step 5: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: Debug app builds.

- [ ] **Step 6: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: report list item activation"
```

---

### Task 5: MainWindow Navigation Integration

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/ui/FinderChrome.cpp` only if integration exposes draw/hit issues

- [ ] **Step 1: Add navigation state to MainWindow header**

Modify `src/ui/MainWindow.h`:

```cpp
#include "navigation/NavigationHistory.h"
#include "navigation/SidebarModel.h"
```

Add:

```cpp
enum class HistoryMode {
    Initial,
    Push,
    Replace,
    BackForward
};
```

Add private methods:

```cpp
    bool navigateToDirectory(const std::wstring& path, HistoryMode mode);
    void activateNode(NodeId nodeId);
    void openFile(const std::wstring& path);
    void refreshChromeState();
    void goBack();
    void goForward();
    void setStatusText(std::wstring text);
```

Add members:

```cpp
    NavigationHistory history_;
    SidebarModel sidebar_;
    ChromeState chromeState_;
    std::wstring homePath_;
```

- [ ] **Step 2: Initialize through navigation**

Replace `initializeFileTree()` implementation in `src/ui/MainWindow.cpp`:

```cpp
void MainWindow::initializeFileTree() {
    homePath_ = defaultHomeDirectory();
    navigateToDirectory(homePath_, HistoryMode::Initial);
}
```

Add helper:

```cpp
void MainWindow::refreshChromeState() {
    chromeState_.title = fileNameFromPath(history_.currentPath());
    if (chromeState_.title.empty()) {
        chromeState_.title = history_.currentPath();
    }
    chromeState_.pathText = history_.currentPath();
    chromeState_.canGoBack = history_.canGoBack();
    chromeState_.canGoForward = history_.canGoForward();
    sidebar_.refresh(homePath_, history_.currentPath());
    chromeState_.sidebarItems = sidebar_.items();
}
```

- [ ] **Step 3: Implement directory navigation**

Add:

```cpp
bool MainWindow::navigateToDirectory(const std::wstring& path, HistoryMode mode) {
    if (path.empty()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    const DirectoryLoadResult result = directoryLoader_.loadChildrenWithStatus(path);
    if (!result.ok()) {
        setStatusText(L"Cannot open folder");
        return false;
    }

    std::wstring rootName = fileNameFromPath(path);
    if (rootName.empty()) {
        rootName = path;
    }

    tree_ = FileTree(path, rootName);
    tree_.replaceChildren(tree_.rootId(), result.children);

    switch (mode) {
    case HistoryMode::Initial:
        history_.setInitialPath(path);
        break;
    case HistoryMode::Push:
        history_.navigateTo(path);
        break;
    case HistoryMode::Replace:
        history_.setInitialPath(path);
        break;
    case HistoryMode::BackForward:
        break;
    }

    chromeState_.statusText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
    return true;
}
```

- [ ] **Step 4: Implement activation and file open**

Add:

```cpp
void MainWindow::activateNode(NodeId nodeId) {
    if (nodeId == kInvalidNodeId || nodeId >= tree_.nodes().size()) {
        return;
    }

    const FileNode& node = tree_.node(nodeId);
    if (node.kind == FileKind::Folder) {
        navigateToDirectory(node.path, HistoryMode::Push);
        return;
    }

    openFile(node.path);
}

void MainWindow::openFile(const std::wstring& path) {
    HINSTANCE result = ShellExecuteW(hwnd_, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        setStatusText(L"Cannot open file");
    }
}

void MainWindow::setStatusText(std::wstring text) {
    chromeState_.statusText = std::move(text);
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

- [ ] **Step 5: Implement back and forward**

Add:

```cpp
void MainWindow::goBack() {
    if (!history_.canGoBack()) {
        return;
    }

    const std::wstring target = history_.backTarget();
    if (navigateToDirectory(target, HistoryMode::BackForward)) {
        history_.goBack();
        refreshChromeState();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

void MainWindow::goForward() {
    if (!history_.canGoForward()) {
        return;
    }

    const std::wstring target = history_.forwardTarget();
    if (navigateToDirectory(target, HistoryMode::BackForward)) {
        history_.goForward();
        refreshChromeState();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}
```

This keeps history unchanged if a historical path is no longer loadable, because the history stack only moves after the target directory loads.

- [ ] **Step 6: Wire mouse handling**

Update `WM_LBUTTONDOWN`:

```cpp
        const ChromeHitResult chromeHit = chrome_.hitTest(x, y, rects, chromeState_);
        if (chromeHit.kind == ChromeHitKind::Back) {
            goBack();
            return 0;
        }
        if (chromeHit.kind == ChromeHitKind::Forward) {
            goForward();
            return 0;
        }
        if (chromeHit.kind == ChromeHitKind::SidebarItem && chromeHit.sidebarIndex < chromeState_.sidebarItems.size()) {
            navigateToDirectory(chromeState_.sidebarItems[chromeHit.sidebarIndex].path, HistoryMode::Push);
            return 0;
        }

        const ListInteractionResult result = listView_.onMouseDown(x, y, rects.list);
        loadChildrenIfNeeded(result.expandedFolder);
        activateNode(result.activatedNode);
        if (result.changed) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
```

- [ ] **Step 7: Wire keyboard handling**

Update `WM_KEYDOWN`:

```cpp
    case WM_KEYDOWN: {
        if (wParam == VK_BACK) {
            goBack();
            return 0;
        }
        const ListInteractionResult result = listView_.onKeyDown(wParam);
        loadChildrenIfNeeded(result.expandedFolder);
        activateNode(result.activatedNode);
        if (result.changed) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
```

- [ ] **Step 8: Draw dynamic chrome state**

Update `paint()`:

```cpp
    chrome_.draw(render_, rects, chromeState_);
```

- [ ] **Step 9: Build and run tests**

Run:

```powershell
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

- [ ] **Step 10: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui src/navigation CMakeLists.txt
git -c safe.directory=D:/finderX commit -m "feat: wire finder navigation"
```

---

### Task 6: Verification And Release Build

**Files:**
- Modify only if verification exposes a defect.

- [ ] **Step 1: Configure**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
```

Expected: configure and generate succeed.

- [ ] **Step 2: Build Debug targets**

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

- [ ] **Step 5: Manual app check**

Run:

```powershell
.\build\Debug\FinderX.exe
```

Check:

- Startup shows files and folders from the Windows user home directory.
- Toolbar title shows the current directory name.
- Bottom path bar shows the current path.
- Sidebar Home/Desktop/Documents/Downloads switch directories when available.
- Double-clicking a folder enters that folder.
- Pressing Enter on a folder enters that folder.
- Pressing Backspace returns to the previous directory.
- Toolbar Back/Forward navigate history when enabled.
- Double-clicking a file attempts to open it with the default app.
- Disclosure arrows still expand folders inside the current directory.
- Wheel scrolling and Up/Down selection still work.
- Empty or access-denied folders do not crash the app.

- [ ] **Step 6: Static checks**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected: no whitespace errors; only intentional untracked design artifacts may remain.

- [ ] **Step 7: Commit verification fixes if needed**

If fixes were needed:

```powershell
git -c safe.directory=D:/finderX add CMakeLists.txt src
git -c safe.directory=D:/finderX commit -m "fix: stabilize navigation usability"
```

If no fixes were needed, do not create an empty commit.
