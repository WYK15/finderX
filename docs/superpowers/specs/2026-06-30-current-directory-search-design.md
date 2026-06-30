# FinderX Current Directory Search Design

## Goal

Make the existing toolbar search field functional by filtering the current visible directory list as the user types. This is a local, current-directory filter, not a recursive search engine or full-disk index.

## Scope

Included:

- Click the toolbar search field to focus search input.
- `Ctrl+F` focuses the search field.
- Typing text updates the search query.
- Backspace deletes text while search is focused.
- Escape clears the query when non-empty; otherwise it leaves search focus.
- The file list filters currently loaded visible rows by file or folder name.
- Filtering is case-insensitive.
- Multi-select, right-click, keyboard navigation, and file operations apply only to filtered visible rows.
- Clearing the query restores the full current list.
- Search query is cleared when navigating to a different directory.

Excluded:

- Recursive search.
- Full-disk indexing.
- Background indexing service.
- File content search.
- Fuzzy matching.
- Search suggestions.
- Search result ranking.
- External Windows Search integration.

## Architecture

Keep search state at the `MainWindow` level and list filtering inside `FinderListView`.

- `MainWindow` owns:
  - current search query text
  - whether the search field has input focus
  - keyboard routing for text input and search shortcuts
- `FinderChrome` owns:
  - drawing the search field with the current query and focus styling
  - hit testing the search rectangle
- `FinderListView` owns:
  - filtering flattened rows before hit testing, drawing, selection, and keyboard navigation

This keeps rendering, input routing, and selection behavior separated.

## Components

### ChromeState And FinderChrome

Extend `ChromeState` with:

- `std::wstring searchText`
- `bool searchFocused`

Extend `ChromeHitKind` with:

- `SearchField`

`FinderChrome::draw` should render:

- placeholder `⌕ Search` when `searchText` is empty and not focused
- current query text when non-empty
- a visible focused outline or subtly different border when focused

`FinderChrome::hitTest` returns `SearchField` when the search box is clicked.

### FinderListView

Add search filtering API:

- `void setFilterText(std::wstring text)`
- `const std::wstring& filterText() const`
- `bool hasFilter() const`

Filtering behavior:

- The filter applies to row names only: `FileNode::name`.
- Matching is case-insensitive.
- If the filter is empty, all flattened rows are visible.
- If the filter is non-empty, only rows whose names contain the query are visible.
- Folder hierarchy is not expanded recursively for matching in this MVP.
- Existing selection is pruned to filtered visible rows.
- `selectAllVisible` selects only filtered rows.
- Mouse hit testing and keyboard navigation operate only on filtered rows.

Implementation note: `FinderListView::rebuildRows()` should continue flattening the tree, then apply the filter before assigning `rows_`.

### MainWindow

Add search state:

- `std::wstring searchText_`
- `bool searchFocused_`

Input behavior:

- Clicking the search field focuses search.
- `Ctrl+F` focuses search and selects the current query conceptually; because there is no full text selection UI yet, new typing may append unless the user presses Escape or Backspace.
- `WM_CHAR` handles printable text while search is focused.
- `WM_KEYDOWN` handles Backspace and Escape while search is focused.
- Escape:
  - if query is non-empty, clear query and keep focus
  - if query is empty, leave search focus
- Enter while search is focused moves focus back to the list.
- Normal list shortcuts should not fire while search is focused except Escape behavior.

Search updates:

- When search text changes, call `listView_.setFilterText(searchText_)`.
- Refresh chrome state and invalidate.
- Clearing search restores the full visible list.
- Navigating to a new directory clears the search query and focus.

### Selection And Operations

When a filter is active:

- `selectedNodes()` returns selected filtered-visible rows only.
- Right-click can target only filtered-visible rows.
- Copy, Cut, Move to Trash, Rename, Open, Show in Explorer, and Copy Path operate on filtered-visible selections.
- `Ctrl+A` selects all filtered-visible rows.

If filtering removes the selected rows, selection should fall back to a valid filtered row when available or become empty.

## Interaction Behavior

- Click search field: focus search.
- `Ctrl+F`: focus search.
- Text input: update filter immediately.
- Backspace while search focused: delete one character.
- Escape while search focused and query non-empty: clear query.
- Escape while search focused and query empty: return focus to list.
- Enter while search focused: return focus to list.
- Clicking the list returns focus to the list.
- Navigating sidebar/back/forward/open folder clears search.

## Error Handling

Search input should never change navigation history or mutate files.

- Invalid or empty query states are safe.
- If filtering produces no rows, the list simply shows no file rows.
- File operation commands with no visible selection continue to no-op.

## Testing And Verification

Automated tests:

- Extend `finderx_list_view_tests`:
  - empty filter shows all visible rows
  - case-insensitive filter by filename
  - unmatched filter has no visible selection
  - `selectAllVisible` respects filter
  - clearing filter restores visible rows
- Add focused tests for any pure search helper if one is introduced.

Manual checks:

- Click search box and type a filename fragment.
- `Ctrl+F` focuses the search box.
- Backspace updates results.
- Escape clears search, then exits search focus.
- Ctrl+A with active search selects only filtered rows.
- Right-click and file operations target filtered rows.
- Navigating folders clears search.

## Acceptance Criteria

- Toolbar search field accepts text input.
- Current directory list filters by case-insensitive name contains match.
- Selection and file operations respect the filtered visible list.
- Clearing search restores the full list.
- Navigation clears search.
- Existing browsing, multi-select, and file operations continue working.
- Debug and Release builds succeed.
- Automated tests pass.
