# FinderX Tab Close, Direct Favorites, And Shortcut Help Design

## Goal

Add three focused usability improvements to the current FinderX feature branch:

1. Let users add a right-clicked file or folder directly to Favorites.
2. Let users close tabs with `Ctrl+W` and a visible `x` on each tab.
3. Show a read-only shortcut list in Settings.

Custom shortcut binding is intentionally out of scope for this iteration.

## Current Context

FinderX already supports:

- `Ctrl+T` to open a new tab.
- Left-clicking tab headers to activate tabs.
- Adding the current directory to Favorites from right-clicking the list background.
- Removing Favorites from the sidebar context menu.
- A Settings dialog for font size and icon size.

The new work should extend these existing paths instead of introducing a separate tab manager or shortcut subsystem.

## Feature Design

### Direct Add To Favorites

The list context menu should show `Add to Favorites` when the user right-clicks a file or folder that is not already a favorite. The favorite target is the selected context node path.

For list background right-clicks, the existing behavior remains: `Add to Favorites` adds the current directory when it is not already a favorite.

The label should use `fileNameFromPath(path)`, falling back to the full path when the filename is empty. Adding should call the existing settings favorite helpers, save settings, refresh chrome/sidebar state, and invalidate the window. Save failure should surface through existing status text behavior.

### Tab Closing

Each visible tab should draw a small `x` close affordance at the right side of the tab. Clicking the `x` closes that tab instead of activating it.

`Ctrl+W` should close the active tab. Closing behavior:

- If multiple tabs remain, remove the requested tab.
- If the closed tab was active, activate the nearest remaining tab, preferring the tab now at the same index, otherwise the previous tab.
- If the last tab is closed, create a new Home tab so FinderX never remains in a no-tab state.
- Restart or stop the directory watcher according to the new active tab location.
- Clear stale context menu state and refresh chrome.

The tab close hit target should be tested independently from normal tab activation.

### Settings Shortcut List

The Settings dialog should include a read-only shortcut list under the existing font and icon inputs. It should list the current built-in shortcuts:

- `Ctrl+T` New tab
- `Ctrl+W` Close tab
- `Ctrl+F` Search
- `F5 / Ctrl+R` Refresh
- `F2` Rename
- `Delete` Move to Trash
- `Ctrl+C` Copy
- `Ctrl+X` Cut
- `Ctrl+V` Paste

This is informational only. It should not create new settings fields, persist shortcut data, or allow editing shortcut bindings.

## Components

- `FinderChrome`: draw tab close affordances and return a new hit kind for close clicks.
- `MainWindow`: close tabs, handle `Ctrl+W`, route close hit results, add right-clicked nodes to Favorites.
- `SettingsDialog`: add read-only shortcut text to the dialog layout.
- Tests: extend chrome tests for close hit targets and add focused tests where practical for tab close helper behavior or menu-independent favorite path selection.

## Error Handling

- Adding a duplicate favorite is a no-op and should not show duplicate menu actions.
- Save failures reuse the current `Cannot save settings` status behavior.
- Closing an invalid tab index is a no-op.
- Closing the last tab creates Home; if Home cannot be opened, FinderX should keep the current tab until navigation succeeds rather than leaving `tabs_` empty.

## Non-Goals

- Editable/custom shortcut bindings.
- Reordering tabs.
- Middle-click tab close.
- Drag-and-drop Favorites.
- MSI or installer changes.

## Verification

Run focused builds and tests after implementation:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Debug --target FinderX finderx_chrome_tests finderx_settings_dialog_tests
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Debug -R "finderx_chrome_tests|finderx_settings_dialog_tests" --output-on-failure
git -c safe.directory=D:/finderX/.worktrees/settings-sort-favorites diff --check
```

Before finishing the branch, rerun full Debug and Release build/test verification.
