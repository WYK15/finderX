# FinderX Multi-Tab MVP Design

## Goal

Add a Finder-like multi-tab experience to FinderX with `Ctrl+T` as the primary entry point. The first version should make multiple folder contexts visible and switchable without expanding into tab management features that are not needed yet.

## User Experience

FinderX will show an independent tab strip above the existing toolbar. The tab strip uses the selected A layout from brainstorming:

- Active tab: white surface, connected visually to the content area.
- Inactive tabs: muted gray surface.
- New tab button: a compact `+` button at the right edge of the visible tabs.
- Tab title: current folder display name, falling back to the full path when a friendly name is unavailable.

Keyboard and mouse behavior:

- `Ctrl+T` creates a new tab.
- Clicking `+` creates a new tab.
- Clicking an existing tab activates it.
- New tabs open at the current active tab's directory.
- If no active path is available, new tabs open at the default home directory.

The first version will not support closing tabs, dragging/reordering tabs, restoring previous sessions, moving tabs across windows, or tab overflow scrolling. Those should be separate follow-up features after the core state model is stable.

## State Model

Introduce a tab state model that owns the per-tab browsing context.

Each tab stores:

- Current `FileTree`.
- Current `FinderListView` state.
- Current `NavigationHistory`.
- Search text and search focus state.
- Last selected path or selected nodes when refreshing or switching.
- Status text for that tab.

`MainWindow` keeps:

- A tab collection that preserves each tab object's address, preferably `std::vector<std::unique_ptr<TabState>> tabs_`.
- `std::size_t activeTabIndex_`.
- Shared services such as `DirectoryLoader`, `FinderChrome`, `SidebarModel`, shell operation state, and render context.

The active tab is the only tab drawn into the file list. Inactive tabs keep their own state in memory but do not run directory watchers while inactive.

`FinderListView` currently stores a `FileTree*`, so `TabState` must not be moved in a way that leaves the list view pointing at an old tree address. The MVP should either keep each `TabState` behind a stable pointer or explicitly rebind the list view after moves. The preferred implementation is stable tab allocation with `std::unique_ptr<TabState>`.

## Navigation And Refresh

Existing navigation actions will operate on the active tab only:

- Back/forward.
- Sidebar navigation.
- Double-click folder navigation.
- Manual refresh.
- Search filtering.
- File actions and context menus.

Directory watcher behavior:

- Only the active tab owns an active watcher.
- Activating another tab stops the old watcher and starts a watcher for the newly active tab's current path.
- Automatic refresh applies only to the active tab.

This keeps watcher behavior simple and avoids background refreshes mutating hidden tabs unexpectedly.

## Chrome And Hit Testing

`FinderChrome` will be extended with tab strip rendering and hit testing.

Layout changes:

- Add a tab strip region above the toolbar.
- Shift the toolbar, header, list, and pathbar down by the tab strip height.
- Keep the sidebar full height so it still feels like Finder's left rail.

Chrome state additions:

- A vector of tab display titles.
- Active tab index.

Hit testing additions:

- `ChromeHitKind::Tab`.
- `ChromeHitKind::NewTab`.
- Hit result tab index for tab hits.

The `+` button is part of chrome hit testing, not a separate native control.

## File Operations

File operations continue to target the active tab.

- Copy/cut state remains window-level, so copying from one tab and pasting into another tab is allowed.
- Rename, trash, open, reveal, copy path, and context menu actions use the active tab's selection/context node.
- After a successful operation, only the active tab refreshes.

This mirrors common file-manager behavior and avoids duplicating clipboard-like operation state per tab.

## Error Handling

If new-tab creation cannot load the target directory:

- Do not add a broken tab.
- Show the existing `Cannot open folder` status message on the active tab.

If a tab's current directory becomes unavailable while it is active:

- Manual or automatic refresh shows `Cannot refresh folder`.
- The tab remains open so the user can navigate elsewhere or close support can be added later.

If tab activation points to an invalid index, ignore it.

## Testing

Add focused tests for the pure tab state model:

- A new manager starts with one tab when initialized.
- `Ctrl+T` equivalent creates a tab at the current path.
- Activating a tab updates the active index.
- Invalid activation is ignored.
- Per-tab search text/history are isolated.

Existing list, search, navigation, file operation, DPI, and directory refresh tests should continue passing.

Manual smoke verification:

- `Ctrl+T` opens a second visible tab at the current directory.
- Clicking tabs switches list content and title state.
- Sidebar navigation affects only the active tab.
- Search text is isolated between tabs.
- External directory changes refresh only the active tab.

## Implementation Boundaries

In scope for MVP:

- Independent visual tab strip.
- `Ctrl+T`.
- `+` hit target.
- Mouse tab switching.
- Per-tab navigation/search/list state.
- Active-tab-only directory watcher.

Out of scope for MVP:

- Closing tabs.
- Reordering tabs.
- Tab overflow scrolling.
- Persistent sessions.
- Cross-window tab movement.
- Middle-click behavior.
- Drag-and-drop files between tabs.
