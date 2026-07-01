# FinderX Address Input Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an address input mode so the user can type a Windows path and navigate directly.

**Architecture:** Store address editing state per tab, parallel to existing search state. Extend `FinderChrome` hit testing with an address field target and render the path bar as editable text while address mode is active. Use existing `navigateToLocation` and status handling for navigation and errors.

**Tech Stack:** C++20, Win32 messages, Direct2D chrome rendering, existing CTest targets.

---

## File Structure

- Modify `src/ui/FinderChrome.h`: add address hit kind and `ChromeState` address fields if they are declared there through included state types.
- Modify `src/navigation/SidebarModel.h`: extend `ChromeState` with address input fields if that is where the struct currently lives.
- Modify `src/ui/FinderChrome.cpp`: draw address input and hit test path bar blank/text area.
- Modify `src/ui/FinderChromeTests.cpp`: test address hit behavior and path bar segment precedence.
- Modify `src/ui/MainWindow.h`: add helper methods for address focus, commit, cancel, and text edits.
- Modify `src/ui/MainWindow.cpp`: wire mouse and keyboard events to address mode.

---

### Task 1: Chrome Address Hit Target

**Files:**
- Modify: `src/ui/FinderChrome.h`
- Modify: `src/ui/FinderChrome.cpp`
- Test: `src/ui/FinderChromeTests.cpp`

- [ ] **Step 1: Write the failing test**

Add this test to `src/ui/FinderChromeTests.cpp`:

```cpp
void testPathbarBlankAreaStartsAddressEditing() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.pathText = L"D:\\alpha\\beta";

    const ChromeHitResult blankHit = chrome.hitTest(930.0f, 684.0f, rects, state);
    require(blankHit.kind == ChromeHitKind::AddressField,
            "pathbar blank area should start address editing");

    const ChromeHitResult segmentHit = chrome.hitTest(244.0f, 684.0f, rects, state);
    require(segmentHit.kind == ChromeHitKind::PathSegment,
            "path segment should keep precedence over address editing");
}
```

Call it from `main()` after `testPathbarSegmentsReturnNavigationTargets()`.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_chrome_tests
```

Expected: compile failure because `ChromeHitKind::AddressField` does not exist.

- [ ] **Step 3: Add the hit kind**

In `src/ui/FinderChrome.h`, add `AddressField` after `PathSegment`:

```cpp
    HeaderKind,
    PathSegment,
    AddressField
```

- [ ] **Step 4: Implement minimal hit testing**

In `FinderChrome::hitTest`, after path segment hit testing and before `return {};`, add:

```cpp
    if (state.statusText.empty() && containsPoint(rects.pathbar, x, y)) {
        return {ChromeHitKind::AddressField, 0, 0, {}};
    }
```

Keep the existing path segment loop before this block so segment clicks navigate rather than start editing.

- [ ] **Step 5: Run test to verify it passes**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_chrome_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug -R "finderx_chrome_tests" --output-on-failure
```

Expected: `finderx_chrome_tests` passes.

---

### Task 2: Address State And Rendering

**Files:**
- Modify: `src/navigation/SidebarModel.h`
- Modify: `src/ui/FinderChrome.cpp`
- Test: `src/ui/FinderChromeTests.cpp`

- [ ] **Step 1: Write the failing test**

Add this test:

```cpp
void testAddressEditingStillHitsAddressField() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.pathText = L"D:\\alpha";
    state.addressEditing = true;
    state.addressText = L"D:\\alpha\\typed";

    const ChromeHitResult hit = chrome.hitTest(244.0f, 684.0f, rects, state);
    require(hit.kind == ChromeHitKind::AddressField,
            "address editing should make the whole pathbar an address field");
}
```

Call it from `main()`.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_chrome_tests
```

Expected: compile failure because `addressEditing` and `addressText` do not exist.

- [ ] **Step 3: Add state fields**

In `ChromeState`, add:

```cpp
    bool addressEditing = false;
    std::wstring addressText;
```

- [ ] **Step 4: Render editing state**

In `FinderChrome::draw`, where the path bar is drawn, branch before normal path segment rendering:

```cpp
    if (state.addressEditing) {
        render.fillRoundedRect(
            D2D1::RoundedRect(pathRect, 6.0f, 6.0f),
            D2D1::ColorF(1.0f, 1.0f, 1.0f));
        render.drawRoundedRect(
            D2D1::RoundedRect(pathRect, 6.0f, 6.0f),
            D2D1::ColorF(0.10f, 0.45f, 0.92f),
            1.2f);
        drawTextClipped(render, state.addressText, D2D1::RectF(pathRect.left + 8.0f, pathRect.top, pathRect.right - 8.0f, pathRect.bottom), window, render.textFormat(), D2D1::ColorF(0.16f, 0.16f, 0.16f));
    } else if (state.statusText.empty()) {
        drawPathSegments(render, clampRect(pathRect, window), state.pathText);
    } else {
        ...
    }
```

- [ ] **Step 5: Make editing hit test take precedence**

At the start of pathbar hit testing in `FinderChrome::hitTest`, before path segment checks:

```cpp
    if (state.addressEditing && containsPoint(rects.pathbar, x, y)) {
        return {ChromeHitKind::AddressField, 0, 0, {}};
    }
```

- [ ] **Step 6: Run test to verify it passes**

Run the chrome test command from Task 1.

---

### Task 3: MainWindow Address Editing

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add helper declarations**

In `MainWindow.h`, add private methods:

```cpp
    void focusAddress();
    void blurAddress();
    void commitAddress();
    bool handleAddressCharacter(wchar_t character);
    bool handleAddressKeyDown(WPARAM key);
```

Add to `TabState`:

```cpp
        std::wstring addressText;
        bool addressEditing = false;
```

- [ ] **Step 2: Route chrome click**

In the `WM_LBUTTONDOWN` switch, add:

```cpp
        case ChromeHitKind::AddressField:
            focusAddress();
            return 0;
```

- [ ] **Step 3: Implement focus and blur**

In `MainWindow.cpp`, near search helpers, add:

```cpp
void MainWindow::focusAddress() {
    if (!hasActiveTab() || activeTab().locationKind == TabState::LocationKind::ThisPc) {
        return;
    }
    TabState& tab = activeTab();
    tab.searchFocused = false;
    tab.addressEditing = true;
    tab.addressText = tab.history.currentPath();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::blurAddress() {
    if (!hasActiveTab()) {
        return;
    }
    TabState& tab = activeTab();
    if (!tab.addressEditing) {
        return;
    }
    tab.addressEditing = false;
    tab.addressText.clear();
    refreshChromeState();
    InvalidateRect(hwnd_, nullptr, FALSE);
}
```

- [ ] **Step 4: Implement commit**

Add:

```cpp
void MainWindow::commitAddress() {
    if (!hasActiveTab() || !activeTab().addressEditing) {
        return;
    }
    std::wstring target = activeTab().addressText;
    blurAddress();
    if (!navigateToLocation(std::move(target), HistoryMode::Push)) {
        setStatusText(L"Cannot open folder");
    }
}
```

- [ ] **Step 5: Implement keyboard handling**

In `WM_KEYDOWN`, before search/list handling, add:

```cpp
        if (hasActiveTab() && activeTab().addressEditing && handleAddressKeyDown(wParam)) {
            return 0;
        }
```

In `WM_CHAR`, before search handling, add:

```cpp
        if (hasActiveTab() && activeTab().addressEditing && handleAddressCharacter(static_cast<wchar_t>(wParam))) {
            return 0;
        }
```

Implement:

```cpp
bool MainWindow::handleAddressKeyDown(WPARAM key) {
    if (!hasActiveTab() || !activeTab().addressEditing) {
        return false;
    }
    if (key == VK_RETURN) {
        commitAddress();
        return true;
    }
    if (key == VK_ESCAPE) {
        blurAddress();
        return true;
    }
    if (key == VK_BACK) {
        std::wstring& text = activeTab().addressText;
        if (!text.empty()) {
            text.pop_back();
            refreshChromeState();
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return true;
    }
    return false;
}

bool MainWindow::handleAddressCharacter(wchar_t character) {
    if (!hasActiveTab() || !activeTab().addressEditing) {
        return false;
    }
    if (character >= 32 && character != 127) {
        activeTab().addressText.push_back(character);
        refreshChromeState();
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
    return true;
}
```

- [ ] **Step 6: Update chrome state**

In `refreshChromeState`, set:

```cpp
    chromeState_.addressEditing = tab.addressEditing;
    chromeState_.addressText = tab.addressEditing ? tab.addressText : L"";
```

Clear address state when navigating or creating tabs, next to search clearing:

```cpp
    tab.addressEditing = false;
    tab.addressText.clear();
```

- [ ] **Step 7: Build**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX finderx_chrome_tests
```

Expected: build succeeds.

---

### Task 4: Verification And Packaging

**Files:**
- No source edits unless verification finds a defect.

- [ ] **Step 1: Run focused tests**

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug -R "finderx_chrome_tests|finderx_list_view_tests|finderx_navigation_tests" --output-on-failure
```

Expected: 3/3 tests pass.

- [ ] **Step 2: Run Release build**

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target FinderX
```

Expected: `D:\finderX\build\Release\FinderX.exe` is updated.

- [ ] **Step 3: Check whitespace**

```powershell
git -c safe.directory=D:/finderX diff --check
```

Expected: exit code 0.

- [ ] **Step 4: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderChrome.h src/ui/FinderChrome.cpp src/ui/FinderChromeTests.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git -c safe.directory=D:/finderX commit -m "feat: add address path input"
```

- [ ] **Step 5: Update portable zip**

Regenerate `dist/FinderX-portable.zip` from the Release executable.

---

## Self-Review

- Spec coverage: this plan covers phase 1 only, address input. Later phases require separate plans.
- Placeholder scan: no unfinished markers or vague implementation-only steps remain.
- Type consistency: `ChromeHitKind::AddressField`, `ChromeState::addressEditing`, and `ChromeState::addressText` are used consistently.
