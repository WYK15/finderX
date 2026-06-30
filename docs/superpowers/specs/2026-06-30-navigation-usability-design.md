# FinderX Navigation Usability Design

## Goal

Make FinderX usable as a real file browser by adding directory navigation, history, sidebar switching, file activation, and dynamic path display while preserving the existing Finder-style list view and lazy tree expansion.

## Scope

This stage prioritizes file-management usability over visual polish and indexing. It includes:

- Startup at the real user home directory.
- Sidebar switching for common user folders.
- Double-click and Enter activation for selected list items.
- Back, Forward, and Backspace directory history.
- Dynamic toolbar title and bottom path bar.
- Default Windows file opening through the shell.
- Error status for navigation or open failures.

This stage does not include search, indexing, MSI packaging, background scanning, breadcrumb clicking, right-click menus, or a large visual redesign.

## Architecture

Navigation state lives near `MainWindow`, but it is split into small model classes so rendering, filesystem loading, and history behavior stay testable.

`FileTree` remains responsible for tree state, node identity, expansion, and flattening. `DirectoryLoader` remains responsible for Win32 directory enumeration. `MainWindow` coordinates UI events, navigation decisions, filesystem loading, and shell file opening.

`FinderChrome` changes from a mostly static renderer into a renderer of `ChromeState`, which contains the current title, path bar text, sidebar items, and Back/Forward enabled state.

## Components

### NavigationHistory

`NavigationHistory` stores:

- `currentPath`
- a back stack
- a forward stack

It exposes:

- `setInitialPath(path)`
- `navigateTo(path)`
- `canGoBack()`
- `canGoForward()`
- `goBack()`
- `goForward()`

Normal navigation pushes the previous current path onto the back stack and clears the forward stack. Back and Forward move paths between stacks without duplicating entries. Invalid or empty navigation requests are ignored by callers before they reach this class.

### SidebarModel

`SidebarModel` builds common folder shortcuts:

- Home
- Desktop
- Documents
- Downloads
- Applications

Each item contains a label, path, availability flag, and selected flag. Home comes from `defaultHomeDirectory()`. Desktop, Documents, and Downloads are derived from the home path. Applications points to a Windows Start Menu Programs location when available; otherwise it is shown unavailable.

Unavailable sidebar rows are drawn disabled and do not navigate.

### FinderListView

`FinderListView` remains the list interaction component. It does not load directories or open files.

It will return explicit activation requests:

- double-clicking a row returns the activated `NodeId`
- pressing Enter returns the selected `NodeId`
- expanding a disclosure still returns an expanded folder for lazy loading

Selection, wheel scrolling, Up/Down, Left/Right expansion behavior, and visible row clipping must remain intact.

### MainWindow

`MainWindow` owns:

- `DirectoryLoader`
- `NavigationHistory`
- `SidebarModel`
- `FileTree`
- `FinderListView`
- `FinderChrome`

It provides:

- `navigateToDirectory(path, historyMode)`
- `activateNode(nodeId)`
- `openFile(path)`
- `loadChildrenIfNeeded(nodeId)`
- `refreshChromeState()`

`navigateToDirectory` loads the target directory first. Only after a successful load does it update history, rebuild `FileTree`, update sidebar selection, clear stale error status, and invalidate the window.

## Data Flow

Directory navigation follows this sequence:

1. A user action produces a target path.
2. `MainWindow::navigateToDirectory` calls `DirectoryLoader::loadChildrenWithStatus`.
3. If loading fails, current directory and history remain unchanged.
4. If loading succeeds, `NavigationHistory` is updated according to the action type.
5. `FileTree` is rebuilt with the target directory as root.
6. Root children are replaced with loaded child nodes.
7. `ChromeState` is refreshed.
8. The window is invalidated.

Tree expansion inside the current directory remains separate:

1. A disclosure click or Right Arrow expands a folder node.
2. `FinderListView` returns the expanded folder `NodeId`.
3. `MainWindow::loadChildrenIfNeeded` lazy-loads that folder.
4. The current directory path and navigation history do not change.

File activation follows this sequence:

1. Double-click or Enter returns an activated `NodeId`.
2. `MainWindow` resolves the node.
3. Folders call `navigateToDirectory`.
4. Files call `ShellExecuteW` with the default `open` verb.

## Interaction Behavior

- Double-clicking a folder enters that folder.
- Double-clicking a file opens it with the Windows default application.
- Pressing Enter activates the selected row.
- Pressing Backspace navigates back when history is available.
- Toolbar Back and Forward buttons navigate through history when enabled.
- Sidebar rows navigate to their folder when available.
- Disclosure arrows expand or collapse a folder inside the current list without changing current directory.
- The bottom path bar displays the current path as readable text.
- The toolbar title displays the current directory name.

## Error Handling

Directory navigation failure:

- Keep current directory unchanged.
- Keep history unchanged.
- Show a short status message such as `Access denied` or `Cannot open folder`.

File opening failure:

- Keep selection and current directory unchanged.
- Show a short status message such as `Cannot open file`.

Unavailable sidebar path:

- Render disabled.
- Do nothing when clicked.

Back or Forward target failure:

- Keep current directory unchanged.
- Keep history unchanged.
- Show an error status.

Empty directories:

- Show an empty list without placeholder rows.

Hidden and system files:

- Keep displaying them in this stage because the current `DirectoryLoader` enumerates them.

## Testing

Automated tests should cover the non-UI models:

- `NavigationHistoryTests`
  - initial path
  - normal navigation
  - forward clearing after new navigation
  - back/forward boundaries
  - no duplicate current path when navigating to the same path

- `SidebarModelTests`
  - Home/Desktop/Documents/Downloads generation from a home path
  - unavailable paths are marked disabled
  - current path marks one item selected

Existing tests must continue to pass:

- `finderx_model_tests`
- `finderx_fs_tests`

Manual app verification must cover:

- startup at Home
- sidebar switching
- double-click folder navigation
- Enter activation
- file opening through the shell
- Backspace return
- toolbar Back/Forward
- path bar updates
- existing disclosure expansion
- scroll and arrow-key selection

## Acceptance Criteria

- FinderX starts at the user's real Home directory.
- Sidebar can switch among available common folders.
- Double-click and Enter enter folders.
- Double-click and Enter open files through the Windows default application.
- Backspace returns to the previous directory.
- Toolbar Back/Forward navigate history.
- Toolbar title and bottom path bar reflect the current directory.
- Existing lazy expansion, selection, scrolling, and arrow keys still work.
- Directory load failures and file open failures do not crash the app.
- Debug and Release `FinderX` builds succeed.
- Model and filesystem tests pass.
