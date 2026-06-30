# FinderX Basic File Actions Design

## Goal

Add basic file actions that make FinderX more useful for daily browsing: refresh current directory, right-click context menu, copy path, reveal in Explorer, and open.

## Scope

Included:

- Right-click a list row to make that row the action target and show a context menu.
- Right-click empty list space to show a smaller menu containing refresh.
- Context menu actions:
  - Open
  - Show in Explorer
  - Copy Path
  - Refresh
- Keyboard refresh:
  - F5
  - Ctrl+R
- Refresh reloads the current directory without changing navigation history.
- Refresh tries to preserve the selected item by path.

Excluded:

- Delete.
- Rename.
- Properties dialog.
- Drag and drop.
- Multi-selection.
- Cut/copy/paste file operations.
- Search and indexing.
- Custom context menu styling.

## Architecture

Keep shell-facing operations out of rendering and list code.

Add a small shell actions layer:

- `ShellActions::openPath(hwnd, path)`
- `ShellActions::revealInExplorer(hwnd, path)`
- `ShellActions::copyPathToClipboard(hwnd, path)`

`MainWindow` owns the context menu flow and keyboard shortcuts. `FinderListView` only exposes hit testing and selection helpers so `MainWindow` can choose the right target.

## Components

### ShellActions

`ShellActions` wraps Win32 shell behavior:

- `openPath` uses `ShellExecuteW(hwnd, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL)`.
- `revealInExplorer` uses Explorer selection for files and folders when possible.
- `copyPathToClipboard` writes a Unicode path string to the Windows clipboard.

The functions return `bool` for success. `MainWindow` decides how to display failures.

### FinderListView

Add list targeting helpers:

- `NodeId nodeAtPoint(float x, float y, const D2D1_RECT_F& bounds)`
- `bool selectNode(NodeId id)`

`nodeAtPoint` rebuilds rows, applies the same hit-test math used by left click, and returns `kInvalidNodeId` when the point is not over a row. It does not change selection.

`selectNode` selects a visible node and returns whether selection changed. It returns false for invalid or non-visible nodes.

### MainWindow

Add context menu state:

- menu command ids for Open, Reveal in Explorer, Copy Path, Refresh
- `NodeId contextNode_`

Handle:

- `WM_RBUTTONDOWN`
  - Convert mouse location.
  - Ask `FinderListView::nodeAtPoint`.
  - If a row was hit, select it and store `contextNode_`.
  - Show a popup menu at the screen coordinate.
  - Row menu contains Open, Show in Explorer, Copy Path, separator, Refresh.
  - Empty menu contains Refresh only.

- `WM_COMMAND`
  - Dispatch menu commands.
  - Open uses existing activation behavior or `ShellActions::openPath`.
  - Show in Explorer calls `ShellActions::revealInExplorer`.
  - Copy Path calls `ShellActions::copyPathToClipboard`.
  - Refresh calls `refreshCurrentDirectory`.

- `WM_KEYDOWN`
  - F5 refreshes.
  - Ctrl+R refreshes.

### Refresh Behavior

`refreshCurrentDirectory`:

1. Reads `history_.currentPath()`.
2. Records selected node path if selection is valid.
3. Reloads the current directory with `DirectoryLoader::loadChildrenWithStatus`.
4. Rebuilds `FileTree` with the same root.
5. Replaces root children.
6. Restores selection by path when possible.
7. Refreshes chrome state and invalidates.

Refresh does not push or modify navigation history.

If refresh fails, the current tree remains unchanged and status text becomes `Cannot refresh folder`.

## Interaction Behavior

- Right-clicking a row selects that row before opening the menu.
- Right-clicking blank list space does not change selection.
- Open on a folder enters the folder.
- Open on a file opens it with the default Windows app.
- Show in Explorer opens Windows Explorer with the item selected when possible.
- Copy Path writes the full path to the clipboard.
- Refresh reloads current directory.

## Error Handling

Failures should not crash or change current directory unexpectedly.

- Open failure: show `Cannot open file`.
- Reveal failure: show `Cannot show in Explorer`.
- Clipboard failure: show `Cannot copy path`.
- Refresh failure: show `Cannot refresh folder`.

Context menu commands with no valid target do nothing except Refresh.

## Testing And Verification

Automated tests:

- Existing tests must continue passing:
  - `finderx_model_tests`
  - `finderx_fs_tests`
  - `finderx_navigation_tests`

Manual checks:

- Right-click row selects it and shows full menu.
- Right-click blank list area shows Refresh only.
- Open menu item opens files or enters folders.
- Show in Explorer opens Explorer for selected item.
- Copy Path places the selected item path on clipboard.
- F5 refresh works.
- Ctrl+R refresh works.
- Refresh does not change Back/Forward history.
- Refresh preserves selection when the selected path still exists.

## Acceptance Criteria

- Right-click row target behavior matches Finder/Explorer expectations.
- Refresh works by menu, F5, and Ctrl+R.
- Copy Path writes Unicode path text to clipboard.
- Show in Explorer works for files and folders.
- Existing navigation, lazy expansion, visual styling, and activation still work.
- Debug and Release `FinderX` builds succeed.
- Existing tests pass.
