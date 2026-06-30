# FinderX File Management Basics Design

## Goal

Add core file-manager affordances that make FinderX usable for everyday folder work before broader settings and sorting features are implemented.

This design covers the first batch only:

- Right-click new folder.
- Right-click new file.
- Right-click open PowerShell in a directory.
- Hide disclosure arrows for folders with no children.
- Add a left-sidebar `This PC` entry so fixed and removable drives are reachable.

Settings, font/icon sizing, favorites, and last-modified sorting are explicitly deferred to a later batch.

## Context Menu Commands

The existing context menu is built in `MainWindow::showContextMenu` and commands are dispatched through `handleCommand`. The new commands will follow that pattern.

Add commands:

- `New Folder`
- `New File`
- `Open in PowerShell`

Menu placement:

- On a file/folder target:
  - Existing open/rename/copy/cut actions remain first.
  - Add `Open in PowerShell` when the target is a folder.
  - Keep reveal/copy path/trash actions grouped as they are.
- On list background:
  - Add `New Folder`
  - Add `New File`
  - Add `Open in PowerShell`
  - Keep paste and refresh behavior.

Target behavior:

- `New Folder` creates inside the active tab's current directory.
- `New File` creates inside the active tab's current directory.
- `Open in PowerShell` opens:
  - selected folder path if the context target is a folder;
  - active tab current directory if right-clicking background or a file.

## New Folder And New File

Default names:

- Folder: `New Folder`
- File: `New Text Document.txt`

Conflict behavior:

- If the base name exists, append a numeric suffix:
  - `New Folder 2`
  - `New Folder 3`
  - `New Text Document 2.txt`
  - `New Text Document 3.txt`

Creation behavior:

- Folder creation uses Win32 filesystem APIs.
- File creation uses Win32 file creation APIs with exclusive create semantics.
- After successful creation:
  - refresh the active tab;
  - select the newly created item;
  - clear any status message.
- On failure:
  - do not refresh unnecessarily;
  - show a short status message such as `Cannot create folder` or `Cannot create file`.

The first version will not show an inline rename prompt after creation. That can be added after the current rename dialog is better integrated with newly created items.

## Open In PowerShell

PowerShell launch should use `powershell.exe` through `ShellExecuteW`.

Working directory:

- If the context target is a folder, use that folder.
- Otherwise use active tab current directory.

Failure behavior:

- If the target directory is empty, unavailable, or no longer exists, show `Cannot open PowerShell`.
- Launch failure also shows `Cannot open PowerShell`.

No command text should be interpolated into a shell command line. Use the working-directory parameter of `ShellExecuteW` instead of building a command string.

## Empty Folder Disclosure Arrows

Current list view draws a disclosure arrow for every folder. This should change:

- Draw an arrow only if the folder has children already known in its `children` vector.
- Do not draw an arrow for known-empty folders.
- Do not toggle expansion when the disclosure area is clicked for a known-empty folder.

Directory loading already eagerly loads children for the active directory. Nested folders may still be unloaded until expanded. For unloaded nested folders, the first version may still show a disclosure arrow because FinderX does not yet know whether they are empty. Once loaded and found empty, the arrow should disappear.

## This PC Sidebar Entry

Add a sidebar entry labeled `This PC`.

Clicking `This PC` should display a drive list in the main pane rather than navigating to a normal filesystem folder.

Drive list behavior:

- Show available drives such as `C:\`, `D:\`, removable drives, and network drives when Windows reports them.
- Each drive appears as a folder-like row.
- Double-clicking a drive navigates to that drive root.
- Sidebar selection marks `This PC` active while the drive list is shown.

Implementation approach:

- Extend the active tab state to support a lightweight virtual location for `This PC`.
- Reuse `FileTree` rows with drive nodes whose paths are drive roots.
- `DirectoryLoader` remains responsible for real directories; a small drive enumeration helper supplies virtual drive rows.

Failure behavior:

- If no drives are returned, show an empty list and keep the path/title as `This PC`.
- If entering a drive root fails, show `Cannot open folder` and keep the current view.

## Testing

Add focused tests for pure helpers where possible:

- Unique new folder/file name generation.
- PowerShell target directory selection.
- Drive enumeration helpers can be smoke-tested where stable, but avoid brittle tests that depend on a specific machine having `D:\`.
- List disclosure behavior should be covered by a list-view test: known-empty folder does not behave as expandable.

Existing tests must continue passing:

- model
- filesystem metadata/loading
- navigation/sidebar
- rename dialog
- file operation state
- tab manager
- directory refresh debouncer
- DPI scaling
- list view

Manual smoke verification:

- Right-click background, create folder, see it appear and selected.
- Right-click background, create file, see it appear and selected.
- Repeat each creation and verify suffixes increment.
- Right-click a folder, open PowerShell in that folder.
- Right-click background, open PowerShell in current folder.
- Confirm known-empty folders do not show an arrow.
- Click `This PC`, see drives, double-click `D:\` when available.

## Out Of Scope

- Inline rename immediately after creating a file/folder.
- User-configurable default file templates.
- Favorites.
- Font and icon size settings.
- Sorting by last modified time.
- Persistent settings.
- Context menu icons.
