# FinderX Tab Close, Direct Favorites, And Shortcut Help Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add direct right-click Favorites, closeable tabs, and a read-only shortcut list in Settings.

**Architecture:** Extend the existing UI routing rather than adding new subsystems. `FinderChrome` owns tab hit-testing and drawing; `MainWindow` owns tab lifecycle and Favorites actions; `SettingsDialog` owns the read-only shortcut display. Keep shortcut customization out of scope.

**Tech Stack:** C++20, Win32, Direct2D/DirectWrite, CMake, existing FinderX test executables.

---

## File Structure

- `src/ui/FinderChrome.h`: add a tab-close hit kind.
- `src/ui/FinderChrome.cpp`: draw close `x` affordances and hit-test them before normal tab activation.
- `src/ui/FinderChromeTests.cpp`: verify tab close hit targets and normal tab hit targets remain distinct.
- `src/ui/MainWindow.h`: declare tab closing and direct favorite helper methods.
- `src/ui/MainWindow.cpp`: handle `Ctrl+W`, tab close hits, context menu favorite target selection, and shortcut dialog wiring.
- `src/ui/SettingsDialog.cpp`: enlarge dialog layout and add a read-only shortcut list.
- `src/ui/SettingsDialogTests.cpp`: keep helper tests; no UI automation required.

---

### Task 1: Direct Right-Click Add To Favorites

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add helper declarations**

In `src/ui/MainWindow.h`, add private helpers near the existing favorite methods:

```cpp
    void addPathToFavorites(const std::wstring& path);
    std::wstring favoriteLabelForPath(const std::wstring& path) const;
```

- [ ] **Step 2: Implement shared favorite helper**

In `src/ui/MainWindow.cpp`, replace duplicated label/add/save logic with:

```cpp
std::wstring MainWindow::favoriteLabelForPath(const std::wstring& path) const {
    std::wstring label = fileNameFromPath(path);
    if (label.empty()) {
        label = path;
    }
    return label;
}

void MainWindow::addPathToFavorites(const std::wstring& path) {
    if (path.empty() || containsFavorite(settings_, path)) {
        return;
    }

    if (!addFavorite(settings_, favoriteLabelForPath(path), path)) {
        return;
    }

    saveSettingsOrStatus();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::addCurrentDirectoryToFavorites() {
    if (!isActiveDirectoryLocation()) {
        return;
    }

    addPathToFavorites(activeTab().history.currentPath());
}
```

- [ ] **Step 3: Add menu action for selected context nodes**

In `showContextMenu`, when `hasTarget` is true and `singleTarget` is true, add `Add to Favorites` for the target node if it is not already a favorite:

```cpp
        if (singleTarget && targets.front() < tab.tree.nodes().size()) {
            const std::wstring& favoritePath = tab.tree.node(targets.front()).path;
            if (!containsFavorite(settings_, favoritePath)) {
                AppendMenuW(menu, MF_STRING, kCommandAddFavorite, L"Add to Favorites");
            }
        }
```

Place it in the target section before the separator that precedes `Show in Explorer`, so file/folder right-clicks expose the action directly.

- [ ] **Step 4: Route Add Favorite command to context target when present**

Update `handleCommand` for `kCommandAddFavorite`:

```cpp
    case kCommandAddFavorite: {
        const NodeId target = commandTargetNode();
        if (target != kInvalidNodeId && hasActiveTab() && target < activeTab().tree.nodes().size()) {
            addPathToFavorites(activeTab().tree.node(target).path);
        } else {
            addCurrentDirectoryToFavorites();
        }
        break;
    }
```

- [ ] **Step 5: Build and smoke the changed target**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_settings_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_settings_tests --output-on-failure
```

Expected: build succeeds and `finderx_settings_tests` passes.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites add src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites commit -m "feat: add selected item favorites"
```

---

### Task 2: Close Tabs With Ctrl+W And Tab X

**Files:**
- Modify: `src/ui/FinderChrome.h`
- Modify: `src/ui/FinderChrome.cpp`
- Modify: `src/ui/FinderChromeTests.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add chrome hit kind**

In `src/ui/FinderChrome.h`, add `CloseTab` after `Tab`:

```cpp
    Tab,
    CloseTab,
    NewTab,
```

Reuse `ChromeHitResult::tabIndex` for the close target.

- [ ] **Step 2: Draw tab close affordance**

In `FinderChrome.cpp`, add a helper:

```cpp
D2D1_RECT_F tabCloseRect(const D2D1_RECT_F& tab) {
    if (tab.right - tab.left < 72.0f) {
        return D2D1::RectF();
    }
    return D2D1::RectF(tab.right - 28.0f, tab.top + 6.0f, tab.right - 8.0f, tab.bottom - 6.0f);
}
```

Inside the tab drawing loop, draw `x` in that rect:

```cpp
        const D2D1_RECT_F closeRect = tabCloseRect(rect);
        if (hasArea(closeRect)) {
            drawTextClipped(
                render,
                L"x",
                D2D1::RectF(closeRect.left + 6.0f, closeRect.top - 1.0f, closeRect.right, closeRect.bottom + 2.0f),
                rect,
                render.textFormat(),
                active ? D2D1::ColorF(0.24f, 0.24f, 0.24f) : D2D1::ColorF(0.50f, 0.50f, 0.50f));
        }
```

Reduce title text right padding so text does not overlap:

```cpp
            D2D1::RectF(rect.left + 12.0f, rect.top + 5.0f, rect.right - 32.0f, rect.bottom - 3.0f),
```

- [ ] **Step 3: Hit-test tab close before tab activation**

In the tab hit-test loop, check close rect before returning `Tab`:

```cpp
        const D2D1_RECT_F closeRect = tabCloseRect(rect);
        if (hasArea(closeRect) && containsPoint(closeRect, x, y)) {
            return {ChromeHitKind::CloseTab, 0, index};
        }
        if (containsPoint(rect, x, y)) {
            return {ChromeHitKind::Tab, 0, index};
        }
```

- [ ] **Step 4: Add chrome tests**

In `src/ui/FinderChromeTests.cpp`, add:

```cpp
void testTabCloseHitTargetDoesNotActivateTab() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.tabTitles = {L"Home", L"Downloads"};
    state.activeTabIndex = 0;

    const ChromeHitResult closeHit = chrome.hitTest(313.0f, 20.0f, rects, state);
    require(closeHit.kind == ChromeHitKind::CloseTab, "tab close x should return close hit kind");
    require(closeHit.tabIndex == 0, "tab close hit should report closed tab index");

    const ChromeHitResult tabHit = chrome.hitTest(220.0f, 20.0f, rects, state);
    require(tabHit.kind == ChromeHitKind::Tab, "tab body should still activate tab");
    require(tabHit.tabIndex == 0, "tab body hit should report tab index");
}
```

Call it from `main()`.

- [ ] **Step 5: Add MainWindow close helpers**

In `src/ui/MainWindow.h`, add:

```cpp
    void closeTab(std::size_t index);
    void ensureActiveTabAfterClose(std::size_t closedIndex);
```

In `src/ui/MainWindow.cpp`, implement:

```cpp
void MainWindow::ensureActiveTabAfterClose(std::size_t closedIndex) {
    if (tabs_.empty()) {
        activeTabIndex_ = 0;
        return;
    }

    if (activeTabIndex_ > closedIndex) {
        --activeTabIndex_;
    } else if (activeTabIndex_ >= tabs_.size()) {
        activeTabIndex_ = tabs_.size() - 1;
    }
}

void MainWindow::closeTab(std::size_t index) {
    if (index >= tabs_.size()) {
        return;
    }

    if (tabs_.size() == 1) {
        const std::wstring fallbackPath = homePath_;
        if (!createTabAtPath(fallbackPath)) {
            return;
        }
        tabs_.erase(tabs_.begin() + static_cast<std::ptrdiff_t>(index));
        activeTabIndex_ = tabs_.empty() ? 0 : tabs_.size() - 1;
    } else {
        tabs_.erase(tabs_.begin() + static_cast<std::ptrdiff_t>(index));
        ensureActiveTabAfterClose(index);
    }

    contextNode_ = kInvalidNodeId;
    contextFavoritePath_.clear();
    if (hasActiveTab() && activeTab().locationKind == TabState::LocationKind::Directory) {
        startDirectoryWatcher(activeTab().history.currentPath());
    } else {
        stopDirectoryWatcher();
    }
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

Add `#include <cstddef>` if needed for `std::ptrdiff_t`.

- [ ] **Step 6: Wire mouse and keyboard**

In `WM_LBUTTONDOWN`, add:

```cpp
        case ChromeHitKind::CloseTab:
            closeTab(chromeHit.tabIndex);
            return 0;
```

In `WM_KEYDOWN`, before search/list handling, add:

```cpp
        if (controlDown && wParam == L'W') {
            if (hasActiveTab()) {
                closeTab(activeTabIndex_);
            }
            return 0;
        }
```

- [ ] **Step 7: Build and test**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_chrome_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_chrome_tests --output-on-failure
```

Expected: build succeeds and `finderx_chrome_tests` passes.

- [ ] **Step 8: Commit**

```powershell
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites add src/ui/FinderChrome.h src/ui/FinderChrome.cpp src/ui/FinderChromeTests.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites commit -m "feat: add closeable tabs"
```

---

### Task 3: Shortcut Help In Settings

**Files:**
- Modify: `src/ui/SettingsDialog.cpp`
- Modify: `src/ui/SettingsDialogTests.cpp`

- [ ] **Step 1: Add shortcut text helper**

In `SettingsDialog.cpp`, add:

```cpp
std::wstring shortcutHelpText() {
    return L"Keyboard shortcuts:\r\n"
           L"Ctrl+T  New tab\r\n"
           L"Ctrl+W  Close tab\r\n"
           L"Ctrl+F  Search\r\n"
           L"F5 / Ctrl+R  Refresh\r\n"
           L"F2  Rename\r\n"
           L"Delete  Move to Trash\r\n"
           L"Ctrl+C  Copy\r\n"
           L"Ctrl+X  Cut\r\n"
           L"Ctrl+V  Paste";
}
```

- [ ] **Step 2: Enlarge settings dialog and add read-only control**

Change dialog height:

```cpp
constexpr int kDialogHeight = 318;
```

Create a read-only multiline edit in `WM_CREATE` after the icon edit:

```cpp
        HWND shortcuts = createDialogControl(WS_EX_CLIENTEDGE,
                                             L"EDIT",
                                             shortcutHelpText().c_str(),
                                             WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                             16,
                                             84,
                                             280,
                                             138,
                                             hwnd,
                                             0);
```

Move OK/Cancel buttons down:

```cpp
                                      136,
                                      236,
                                      76,
                                      28,
```

```cpp
                                          220,
                                          236,
                                          76,
                                          28,
```

Include `shortcuts` in the control creation failure check:

```cpp
        if (!fontLabel || !fontEdit || !iconLabel || !iconEdit || !shortcuts || !ok || !cancel) {
            return -1;
        }
```

- [ ] **Step 3: Add helper test**

Expose the helper only if needed by keeping it internal and instead add a parsing-independent test through a new public function only if necessary. Prefer no public API addition. Keep `SettingsDialogTests.cpp` focused on `applySettingsDialogValues`; the UI layout is covered by build verification.

- [ ] **Step 4: Build and test**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_settings_dialog_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R finderx_settings_dialog_tests --output-on-failure
```

Expected: build succeeds and `finderx_settings_dialog_tests` passes.

- [ ] **Step 5: Commit**

```powershell
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites add src/ui/SettingsDialog.cpp src/ui/SettingsDialogTests.cpp
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites commit -m "feat: show shortcut help in settings"
```

---

### Task 4: Final Verification

**Files:**
- No source changes expected.

- [ ] **Step 1: Run focused Debug build and tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_chrome_tests finderx_settings_dialog_tests finderx_settings_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R "finderx_chrome_tests|finderx_settings_dialog_tests|finderx_settings_tests" --output-on-failure
```

Expected: build succeeds and all focused tests pass.

- [ ] **Step 2: Run full Debug and Release verification**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug --output-on-failure
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

Expected: Debug and Release builds succeed, and all tests pass in both configurations.

- [ ] **Step 3: Check whitespace and status**

Run:

```powershell
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites diff --check
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites status --short
```

Expected: `diff --check` prints no output; `status --short` shows no source changes.

