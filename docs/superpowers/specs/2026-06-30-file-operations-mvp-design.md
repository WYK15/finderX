# FinderX File Operations MVP Design

## Goal

Add the first real file-management actions to FinderX: rename, move to trash, copy, cut, and paste. The goal is to make the app useful for basic daily file management while keeping behavior close to Windows Explorer and macOS Finder expectations.

## Scope

Included:

- Single selected file or folder as the operation target.
- Context menu actions:
  - Rename
  - Copy
  - Cut
  - Paste
  - Move to Trash
- Keyboard shortcuts:
  - F2 for rename
  - Delete for move to trash
  - Ctrl+C for copy
  - Ctrl+X for cut
  - Ctrl+V for paste
- File operations use Windows shell behavior where practical.
- Operations refresh the current directory after completion.
- Rename attempts to select the renamed item after refresh.

Excluded:

- Multi-selection.
- Inline rename editing inside the list.
- Permanent delete and Shift+Delete.
- Drag and drop.
- Undo.
- File operation progress UI.
- Custom conflict resolution UI.
- Search indexing.
- Properties dialog.

## Architecture

Keep real filesystem mutations behind a shell-facing module. `MainWindow` should decide when commands are available and how the UI refreshes, but it should not implement low-level copy, move, delete, or rename behavior directly.

Add a new file-operation layer:

- `ShellFileOperations::renamePath(hwnd, oldPath, newName, renamedPathOut)`
- `ShellFileOperations::moveToTrash(hwnd, path)`
- `ShellFileOperations::copyToDirectory(hwnd, sourcePath, destinationDirectory)`
- `ShellFileOperations::moveToDirectory(hwnd, sourcePath, destinationDirectory)`

Clipboard intent for file operations is held inside FinderX state for the MVP. This avoids implementing full Explorer-compatible `CF_HDROP` clipboard formats immediately. Text path copying remains handled by the existing `ShellActions::copyPathToClipboard`.

## Components

### ShellFileOperations

`ShellFileOperations` wraps Windows APIs for mutating filesystem operations.

- Rename validates that the new name is not empty and does not contain path separators. It renames within the same parent directory.
- Move to Trash must use shell recycle-bin behavior, not permanent deletion.
- Copy and move to directory should use shell file operation behavior where practical, so folders, conflicts, permissions, and system dialogs follow Windows conventions.
- Every function returns a structured result with success state and a short user-facing failure message.

This module must not know about `FileTree`, rendering, selection, or navigation history.

### MainWindow

`MainWindow` owns command routing and command state:

- Track pending file clipboard state:
  - source path
  - operation kind: copy or cut
- Extend the context menu:
  - Row target menu includes Open, Rename, Copy, Cut, Show in Explorer, Copy Path, Move to Trash, separator, Refresh.
  - Empty list menu includes Paste when a pending file operation exists, then Refresh.
  - Row target menu also includes Paste when a pending file operation exists.
- Handle keyboard shortcuts:
  - F2 triggers rename for the selected row.
  - Delete moves the selected row to trash.
  - Ctrl+C stores a copy intent for the selected row.
  - Ctrl+X stores a cut intent for the selected row.
  - Ctrl+V pastes into the current directory when an intent exists.

Commands with no valid target do nothing except Paste and Refresh, which target the current directory.

### Rename Dialog

Use a small native modal dialog for the MVP:

- Shows the current file or folder name in an edit field.
- OK confirms.
- Cancel or Escape aborts without changes.
- Empty names are rejected.
- Names containing `\`, `/`, `:`, `*`, `?`, `"`, `<`, `>`, or `|` are rejected before calling the filesystem.

Inline Finder-style rename is intentionally deferred because it touches list layout, focus, IME behavior, cancel/commit handling, and selection painting.

### Refresh And Selection

After successful operations:

- Rename refreshes and selects the renamed path when possible.
- Move to Trash refreshes and clears stale selection.
- Copy and paste refresh the current directory.
- Cut and paste clears the pending cut intent only after a successful move.
- Copy intent remains after paste, matching common clipboard behavior.

Failed operations must leave the current tree and navigation history unchanged.

## Interaction Behavior

- Right-clicking a row still selects that row before showing the menu.
- Copy and Cut operate on the selected or context row.
- Paste always targets `history_.currentPath()`.
- Delete means Move to Trash.
- Cut is a pending move operation; it does not visually dim the source item in the MVP.
- If the pending source no longer exists when pasting, show an error and keep the current directory unchanged.
- If the destination already has an item with the same name, defer to Windows shell conflict behavior.

## Error Handling

Failures should show concise status text and avoid partial UI state changes.

- Rename failure: `Cannot rename item`
- Move to Trash failure: `Cannot move item to trash`
- Copy failure: `Cannot copy item`
- Move failure: `Cannot move item`
- Paste without pending operation: do nothing
- Paste into unavailable current directory: `Cannot paste here`

If an operation reports failure, FinderX should not refresh unless the shell operation may have partially completed and the current directory needs reconciliation. In that case refresh is allowed, but history must not change.

## Testing And Verification

Automated tests:

- Existing tests must continue passing:
  - `finderx_model_tests`
  - `finderx_fs_tests`
  - `finderx_navigation_tests`
- Add focused tests for non-UI helpers where practical:
  - rename name validation
  - pending file operation state transitions

Manual checks:

- F2 opens rename dialog for selected item.
- Rename rejects invalid names.
- Rename refreshes and selects the renamed item.
- Delete moves a test item to Recycle Bin.
- Ctrl+C then Ctrl+V copies into the current directory.
- Ctrl+X then Ctrl+V moves into the current directory and clears cut state.
- Context menu actions match keyboard shortcuts.
- Paste appears only when a pending copy or cut operation exists.
- Back/Forward history is unchanged after file operations.

## Acceptance Criteria

- Single-item rename, trash, copy, cut, and paste work for files and folders.
- Delete never permanently deletes files.
- File operation commands are available from both context menu and keyboard where specified.
- Current directory refreshes after successful operations.
- Navigation, opening, reveal in Explorer, copy path, and refresh continue working.
- Debug and Release `FinderX` builds succeed.
- Existing automated tests pass.
