# FinderX Multi-Select And Batch Operations Design

## Goal

Add Finder/Explorer-style multi-selection to the file list and extend existing file operations to work on selected groups. The goal is to make FinderX practical for common multi-file workflows without expanding into drag-and-drop, external clipboard formats, or batch rename.

## Scope

Included:

- Multi-select rows in the current visible file list.
- Mouse selection:
  - plain click selects one row
  - Ctrl+click toggles a row in the selection
  - Shift+click selects a contiguous visible range
- Keyboard selection:
  - Up/Down moves the focused row and collapses selection to that row
  - Shift+Up/Shift+Down extends a contiguous selection range
  - Ctrl+A selects all visible rows
- Multi-selected rows use the same Finder-style blue selection treatment.
- One row remains the focused/anchor row for keyboard movement and Shift range selection.
- Context menu behavior:
  - Right-click a selected row keeps the current multi-selection and targets the group.
  - Right-click an unselected row replaces selection with that row and targets it.
  - Right-click blank list space keeps selection unchanged and shows Paste/Refresh.
- Batch operations:
  - Copy selected items.
  - Cut selected items.
  - Move selected items to trash.
  - Paste all pending copied or cut items into the current directory.
- Rename remains enabled only for exactly one selected item.
- Open remains single-target: it opens the focused/context item only.

Excluded:

- Batch rename.
- Drag and drop.
- External Explorer-compatible file clipboard formats such as `CF_HDROP`.
- Cross-window file operation sharing.
- Select-all across collapsed tree descendants or unloaded folders.
- Visual dimming of cut items.
- Progress UI and custom conflict UI.
- Undo.
- Search indexing.

## Architecture

Extend the existing single-selection model rather than replacing the list view. `FinderListView` should own visible-row selection mechanics and expose selected node IDs to `MainWindow`. `MainWindow` should decide which commands operate on a single focused item and which commands operate on the selected set.

The existing in-app pending file operation state should be upgraded from one source path to a vector of source paths. It remains internal to FinderX for this MVP.

## Components

### FinderListView

`FinderListView` becomes the owner of:

- focused row node ID
- anchor row node ID for Shift range selection
- selected node IDs

The existing `selectedNode()` API remains as the focused node for compatibility. New APIs:

- `std::vector<NodeId> selectedNodes() const`
- `bool hasSelection() const`
- `bool isSelected(NodeId id) const`
- `bool selectNodes(std::span<const NodeId> ids)`
- `bool selectAllVisible()`
- `bool clearSelection()`

Selection invariants:

- Selection contains only visible, valid node IDs.
- Focused node is either invalid or a visible selected node.
- Anchor node is either invalid or visible.
- Collapsing a folder removes hidden descendants from selection.
- Reloading a tree resets selection unless `MainWindow` restores it by path.

Mouse behavior:

- Plain click on a row selects only that row and makes it focused and anchor.
- Ctrl+click toggles the clicked row. If toggling off the focused row, focus moves to another selected row when possible; otherwise it becomes the clicked row.
- Shift+click selects the visible range from anchor to clicked row. If no anchor exists, it behaves like plain click.
- Double-click opens only the clicked row, not every selected row.
- Disclosure click keeps selection behavior conservative: if the clicked row is not selected, it becomes the focused single selection before expanding/collapsing.

Keyboard behavior:

- Up/Down moves focus to the adjacent visible row and collapses selection to that row.
- Shift+Up/Shift+Down extends the range from anchor to the new focused row.
- Left/Right expansion behavior continues to operate on the focused row.
- Enter opens only the focused row.
- Ctrl+A selects every visible row and keeps the current focused row when possible.

### FileOperationState

`FileOperationState` stores multiple source paths:

- operation kind: none, copy, cut
- ordered source paths

New or adjusted API:

- `void setCopy(std::vector<std::wstring> paths)`
- `void setCut(std::vector<std::wstring> paths)`
- `const std::vector<std::wstring>& sourcePaths() const`

Single-path helpers may remain for compatibility if useful, but `MainWindow` should use the vector API for file operations.

State behavior:

- Empty path lists clear state.
- Copy remains pending after paste.
- Cut clears only after successful paste.
- Invalid or empty individual paths are ignored before storing. If none remain, state clears.

### ShellFileOperations

Add batch-oriented wrappers that call the existing safe shell operations:

- `FileOperationResult copyToDirectory(HWND owner, std::span<const std::wstring> sourcePaths, const std::wstring& destinationDirectory)`
- `FileOperationResult moveToDirectory(HWND owner, std::span<const std::wstring> sourcePaths, const std::wstring& destinationDirectory)`
- `FileOperationResult moveToTrash(HWND owner, std::span<const std::wstring> paths)`

The single-path APIs may remain and delegate to the batch APIs.

Batch shell operations must preserve the existing safety boundary:

- all source paths must be absolute
- destination directory must be absolute where applicable
- trash delete must use recycle-bin behavior
- failure or user abort returns the existing user-facing messages

The MVP may either pass a multi-string source list to one `SHFileOperationW` call or call the existing single-path wrapper in sequence. One `SHFileOperationW` call is preferred so Windows conflict and progress UI is coherent.

### MainWindow

`MainWindow` switches from command target node to command target nodes for batch operations.

Target rules:

- For Copy, Cut, Move to Trash: use all selected nodes when the context row is selected or no context row is set; use the context row alone when right-clicking an unselected row.
- For Rename: require exactly one target node.
- For Open, Show in Explorer, Copy Path: operate on the focused/context node only for this MVP.
- For Paste: target `history_.currentPath()` and all pending source paths.

Context menu rules:

- Row menu always includes Open.
- Rename is enabled only for single target selection.
- Copy, Cut, Move to Trash apply to one or many selected items.
- Paste appears when a pending file operation exists.
- Blank list menu shows Paste when pending and Refresh.

Refresh behavior:

- After rename, restore selection to the renamed path.
- After trash, refresh and clear stale selection.
- After paste, refresh current directory.
- Normal refresh attempts to preserve selected paths that still exist, not only a single selected path.

## Interaction Behavior

- Multi-select is limited to currently visible rows.
- Collapsed or unloaded children are not selected by Ctrl+A.
- Selection should not include the root placeholder if it is not displayed as a row.
- Right-clicking blank list space does not clear selection.
- Copy/Cut on no selection does nothing.
- Delete on no selection does nothing.
- Paste with no pending operation does nothing.
- Cut state clears only after a successful move operation.

## Error Handling

Failures should show concise status text and avoid changing selection unexpectedly.

- Batch trash failure: `Cannot move items to trash`
- Batch copy failure: `Cannot copy items`
- Batch move failure: `Cannot move items`
- Paste into unavailable current directory: `Cannot paste here`
- Rename with zero or multiple selected items: no-op

If a batch operation may have partially completed, FinderX should refresh the current directory to reconcile state, but it must not change navigation history.

## Testing And Verification

Automated tests:

- Add or extend focused tests for selection mechanics:
  - plain click single selection
  - Ctrl+click toggle
  - Shift+click range
  - Ctrl+A visible rows
  - collapse removes hidden descendants from selection
  - keyboard movement collapses or extends selection as specified
- Extend file operation state tests:
  - copy stores multiple paths and persists after paste
  - cut stores multiple paths and clears after paste
  - empty path list clears
  - empty individual paths are ignored
- Existing tests continue passing:
  - model
  - filesystem
  - navigation
  - rename dialog
  - file operation state

Manual checks:

- Ctrl+click selects multiple rows visually.
- Shift+click selects a visible range.
- Ctrl+A selects visible rows.
- Right-click selected row keeps group selection.
- Right-click unselected row switches to that row.
- Delete moves multiple disposable files to Recycle Bin.
- Ctrl+C then Ctrl+V copies multiple disposable files.
- Ctrl+X then Ctrl+V moves multiple disposable files and clears cut state.
- Rename is available only when one item is selected.
- Open still opens only one focused item.

## Acceptance Criteria

- Users can select multiple visible rows with mouse and keyboard.
- Selected rows are visually clear and Finder-like.
- Copy, Cut, Paste, and Move to Trash support multiple selected files/folders.
- Rename remains single-item.
- Existing single-item workflows continue working.
- Navigation history is unchanged by batch file operations.
- Debug and Release `FinderX` builds succeed.
- Automated tests pass.
