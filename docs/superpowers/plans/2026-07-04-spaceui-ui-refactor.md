# SpaceUI UI Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor FinderX UI rendering to use a SpaceUI-inspired semantic theme token layer and apply it to the chrome, file list, and drag feedback surfaces.

**Architecture:** Add a native `ThemeTokens` module that resolves light/dark semantic colors and radii from `ThemeMode`. Migrate `FinderChrome`, `FinderListView`, and `MainWindow` drag feedback to consume token values rather than scattered hard-coded color pairs.

**Tech Stack:** C++20, Win32, Direct2D, DirectWrite, CMake/CTest.

---

### Task 1: Add Semantic Theme Tokens

**Files:**
- Create: `src/ui/ThemeTokens.h`
- Create: `src/ui/ThemeTokens.cpp`
- Create: `src/ui/ThemeTokensTests.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing tests**

Create tests that assert dark and light tokens are distinct, semantic colors exist for app/sidebar/menu/list surfaces, accent is blue, and radii match the design baseline.

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target finderx_theme_tokens_tests
```

Expected: build fails because the target/module does not exist yet.

- [ ] **Step 3: Implement tokens and CMake target**

Add `ThemeTokens` with semantic fields:

- `accent`, `accentFaint`, `accentDeep`
- `ink`, `inkDull`, `inkFaint`
- `app`, `appBox`, `appOverlay`, `appInput`, `appLine`, `appHover`, `appSelected`, `appActive`
- `sidebar`, `sidebarBox`, `sidebarLine`, `sidebarSelected`
- `menu`, `menuLine`, `menuHover`, `menuSelected`
- `statusError`
- `rowStripe`, `rowSelected`
- `radiusControl`, `radiusPanel`, `radiusWindow`

- [ ] **Step 4: Run test to verify it passes**

Run the same build target and then:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release -R finderx_theme_tokens_tests --output-on-failure
```

Expected: `finderx_theme_tokens_tests` passes.

### Task 2: Migrate FinderChrome To Theme Tokens

**Files:**
- Modify: `src/ui/FinderChrome.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update build inputs**

Add `src/ui/ThemeTokens.cpp` to `finderx_chrome_tests` and `FinderX` sources.

- [ ] **Step 2: Replace local color helpers**

Replace chrome-local semantic color variables with `themeTokens(state.themeMode)` values. Keep geometry and hit-testing unchanged.

- [ ] **Step 3: Preserve Chrome Tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target finderx_chrome_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release -R finderx_chrome_tests --output-on-failure
```

Expected: all chrome hit target tests pass.

### Task 3: Migrate FinderListView And Drag Feedback To Theme Tokens

**Files:**
- Modify: `src/ui/FinderListView.cpp`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update build inputs**

Add `src/ui/ThemeTokens.cpp` to `finderx_list_view_tests` and any other affected UI test targets as needed.

- [ ] **Step 2: Replace list colors**

Use `themeTokens(style_.themeMode)` for row stripe, selection, text, muted text, file/folder icon palette, and disclosure color.

- [ ] **Step 3: Replace drag feedback colors**

Use `themeTokens(settings_.themeMode)` for overlay shadow, panel fill, border, and text.

- [ ] **Step 4: Preserve List Tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target finderx_list_view_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release -R finderx_list_view_tests --output-on-failure
```

Expected: all list interaction tests pass.

### Task 4: Final Verification

**Files:**
- Review: `Design.md`
- Review: `AGENTS.md`
- Review: changed UI files

- [ ] **Step 1: Build Release**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

- [ ] **Step 2: Run full tests**

Run:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

- [ ] **Step 3: Diff review**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX diff --stat
```

Expected: no whitespace errors; changes are limited to the theme refactor plan, theme token module, CMake, and UI files.
