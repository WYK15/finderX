# FinderX Favorites Management Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Settings dialog controls to rename, remove, and reorder favorites.

**Architecture:** Add small testable favorite helper functions to `AppSettings`, then wire those helpers into the existing Win32 `SettingsDialog` state. The sidebar continues to refresh from `settings_.favorites` after settings are accepted.

**Tech Stack:** C++20, Win32 dialog controls, existing AppSettings JSON persistence, CTest.

---

## File Structure

- Modify `src/settings/AppSettings.h`: declare favorite rename and reorder helpers.
- Modify `src/settings/AppSettings.cpp`: implement helper behavior.
- Modify `src/settings/AppSettingsTests.cpp`: add helper tests.
- Modify `src/ui/SettingsDialog.h`: extend dialog value/state support if needed.
- Modify `src/ui/SettingsDialog.cpp`: add list/edit/buttons for favorite management.
- Modify `src/ui/SettingsDialogTests.cpp`: verify size values still apply and favorite edits are preserved.

---

### Task 1: Favorite Helper Functions

**Files:**
- Modify: `src/settings/AppSettings.h`
- Modify: `src/settings/AppSettings.cpp`
- Test: `src/settings/AppSettingsTests.cpp`

- [ ] **Step 1: Write failing tests**

Add tests to `src/settings/AppSettingsTests.cpp`:

```cpp
void testRenameFavoriteByIndex() {
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "favorite should exist before rename");

    require(renameFavorite(settings, 0, L"Work"), "renameFavorite should rename valid index");
    require(settings.favorites[0].label == L"Work", "renameFavorite should update label");
    require(settings.favorites[0].path == L"D:\\Projects", "renameFavorite should keep path");
    require(!renameFavorite(settings, 9, L"Missing"), "renameFavorite should reject invalid index");
}

void testRenameFavoriteEmptyLabelFallsBackToPathName() {
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "favorite should exist before fallback rename");

    require(renameFavorite(settings, 0, L""), "empty rename should still update valid favorite");
    require(settings.favorites[0].label == L"Projects", "empty label should fall back to final path component");
}

void testMoveFavoriteByIndex() {
    AppSettings settings;
    require(addFavorite(settings, L"One", L"D:\\One"), "first favorite should be added");
    require(addFavorite(settings, L"Two", L"D:\\Two"), "second favorite should be added");
    require(addFavorite(settings, L"Three", L"D:\\Three"), "third favorite should be added");

    require(moveFavorite(settings, 2, -1), "moveFavorite should move item up");
    require(settings.favorites[1].label == L"Three", "moved favorite should occupy previous slot");
    require(moveFavorite(settings, 1, 1), "moveFavorite should move item down");
    require(settings.favorites[2].label == L"Three", "moved favorite should occupy next slot");
    require(!moveFavorite(settings, 0, -1), "moveFavorite should reject first item moving up");
    require(!moveFavorite(settings, 2, 1), "moveFavorite should reject last item moving down");
}
```

Call these tests from `main()`.

- [ ] **Step 2: Run tests to verify red**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_settings_tests
```

Expected: compile failure because `renameFavorite` and `moveFavorite` do not exist.

- [ ] **Step 3: Add declarations**

In `src/settings/AppSettings.h`, add:

```cpp
bool renameFavorite(AppSettings& settings, std::size_t index, std::wstring label);
bool moveFavorite(AppSettings& settings, std::size_t index, int direction);
```

Add `#include <cstddef>` if required for `std::size_t`.

- [ ] **Step 4: Implement helpers**

In `src/settings/AppSettings.cpp`, add a path-label helper and implementations:

```cpp
std::wstring favoriteLabelFromPath(const std::wstring& path) {
    const std::size_t end = path.find_last_not_of(L"\\/");
    if (end == std::wstring::npos) {
        return path;
    }
    const std::size_t slash = path.find_last_of(L"\\/", end);
    if (slash == std::wstring::npos) {
        return path.substr(0, end + 1);
    }
    std::wstring label = path.substr(slash + 1, end - slash);
    return label.empty() ? path : label;
}
```

```cpp
bool renameFavorite(AppSettings& settings, std::size_t index, std::wstring label) {
    if (index >= settings.favorites.size()) {
        return false;
    }
    if (label.empty()) {
        label = favoriteLabelFromPath(settings.favorites[index].path);
    }
    settings.favorites[index].label = std::move(label);
    return true;
}

bool moveFavorite(AppSettings& settings, std::size_t index, int direction) {
    if (index >= settings.favorites.size() || (direction != -1 && direction != 1)) {
        return false;
    }
    if (direction < 0 && index == 0) {
        return false;
    }
    const std::size_t next = direction < 0 ? index - 1 : index + 1;
    if (next >= settings.favorites.size()) {
        return false;
    }
    std::swap(settings.favorites[index], settings.favorites[next]);
    return true;
}
```

- [ ] **Step 5: Run tests to verify green**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_settings_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug -R "finderx_settings_tests" --output-on-failure
```

Expected: settings tests pass.

- [ ] **Step 6: Commit**

```powershell
git -c safe.directory=D:/finderX add src/settings/AppSettings.h src/settings/AppSettings.cpp src/settings/AppSettingsTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: add favorite settings helpers"
```

---

### Task 2: Settings Dialog Favorite Controls

**Files:**
- Modify: `src/ui/SettingsDialog.cpp`
- Test: `src/ui/SettingsDialogTests.cpp`

- [ ] **Step 1: Add dialog control constants**

In `SettingsDialog.cpp`, add IDs:

```cpp
constexpr int kFavoritesListId = 201;
constexpr int kFavoriteLabelEditId = 202;
constexpr int kFavoritePathEditId = 203;
constexpr int kFavoriteUpId = 204;
constexpr int kFavoriteDownId = 205;
constexpr int kFavoriteRemoveId = 206;
```

Increase dialog height:

```cpp
constexpr int kDialogHeight = 456;
```

- [ ] **Step 2: Add helper functions**

Add helper functions inside the anonymous namespace:

```cpp
void populateFavoritesList(HWND hwnd, const AppSettings& settings, int selectedIndex);
int selectedFavoriteIndex(HWND hwnd);
void selectFavorite(HWND hwnd, SettingsDialogState& state, int index);
void syncSelectedFavoriteLabel(HWND hwnd, SettingsDialogState& state);
```

Implement them using `LB_RESETCONTENT`, `LB_ADDSTRING`, `LB_SETCURSEL`, `LB_GETCURSEL`, `SetWindowTextW`, and `GetDlgItem`.

- [ ] **Step 3: Create controls**

In `WM_CREATE`, after icon size controls, add:

- Static label `Favorites:`
- Listbox at `16, 92, 280, 96`
- Static label `Name:`
- Edit at `72, 198, 224, 24`
- Static label `Path:`
- Read-only edit at `72, 230, 224, 24`
- Buttons Up, Down, Remove at y `264`
- Move shortcut help lower, and OK/Cancel to the bottom.

Use existing `createDialogControl`.

- [ ] **Step 4: Wire selection and buttons**

In `WM_COMMAND`:

- If `LOWORD(wParam) == kFavoritesListId && HIWORD(wParam) == LBN_SELCHANGE`, call `selectFavorite`.
- If `LOWORD(wParam) == kFavoriteLabelEditId && HIWORD(wParam) == EN_CHANGE`, update current favorite label through `renameFavorite`.
- If Up/Down clicked, call `moveFavorite`, repopulate list, and select new index.
- If Remove clicked, erase selected favorite from `state->resultSettings.favorites`, repopulate, and select next/previous item.

- [ ] **Step 5: Ensure OK uses edited favorites**

On OK:

```cpp
syncSelectedFavoriteLabel(hwnd, *state);
state->values.fontSizeText = getEditText(GetDlgItem(hwnd, kFontEditId));
state->values.iconSizeText = getEditText(GetDlgItem(hwnd, kIconEditId));
applySettingsDialogValues(state->values, state->resultSettings);
```

Do not reset `resultSettings` from `initialSettings` on OK, because favorite controls edit `resultSettings` live.

- [ ] **Step 6: Add regression test for preserving favorites**

In `SettingsDialogTests.cpp`, add:

```cpp
{
    AppSettings settings;
    require(addFavorite(settings, L"Projects", L"D:\\Projects"), "favorite should exist before applying dialog values");
    applySettingsDialogValues(SettingsDialogValues{L"16", L"20"}, settings);
    require(settings.favorites.size() == 1, "applying size values should preserve favorites");
    require(settings.favorites[0].label == L"Projects", "applying size values should preserve favorite label");
}
```

- [ ] **Step 7: Build and test**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX finderx_settings_dialog_tests finderx_settings_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug -R "finderx_settings_dialog_tests|finderx_settings_tests" --output-on-failure
```

Expected: tests pass and FinderX builds.

- [ ] **Step 8: Commit**

```powershell
git -c safe.directory=D:/finderX add src/ui/SettingsDialog.cpp src/ui/SettingsDialogTests.cpp
git -c safe.directory=D:/finderX commit -m "feat: manage favorites in settings"
```

---

### Task 3: Verification And Packaging

**Files:**
- No source edits unless verification fails.

- [ ] **Step 1: Run focused tests**

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Debug -R "finderx_settings_tests|finderx_settings_dialog_tests|finderx_navigation_tests|finderx_chrome_tests" --output-on-failure
```

Expected: all selected tests pass.

- [ ] **Step 2: Build Release**

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target FinderX
```

Expected: `D:\finderX\build\Release\FinderX.exe` is updated.

- [ ] **Step 3: Check whitespace**

```powershell
git -c safe.directory=D:/finderX diff --check
```

Expected: exit code 0.

- [ ] **Step 4: Update portable zip**

Regenerate `dist/FinderX-portable.zip` from `build\Release\FinderX.exe`.

---

## Self-Review

- Spec coverage: rename, remove, and reorder favorites are covered.
- Scope: path editing and drag-and-drop are excluded.
- Type consistency: helper names are consistent across declaration, implementation, and tests.
