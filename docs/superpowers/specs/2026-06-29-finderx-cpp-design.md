# FinderX C++ Design

Date: 2026-06-29

## Goal

FinderX is a Windows-native file browser inspired by macOS Finder list view. The first version focuses on a polished, high-performance hierarchical list: folders expand inline, child files are indented in the same table, and the UI follows the density and structure of the provided Finder reference.

The MVP will be implemented in C++ only. Zig is intentionally deferred to avoid early build, ABI, memory ownership, and debugging complexity.

## Visual Target

The primary reference is macOS Finder in list view:

- Rounded app window.
- Light gray left sidebar.
- Top toolbar with navigation, view controls, sort/group actions, share/more actions, and search.
- Main table with columns: name, modified date, size, kind.
- Inline folder expansion using disclosure arrows.
- Indented descendants in the same list.
- Alternating subtle row backgrounds.
- Full-row blue selected state.
- Bottom path bar.

FinderX should not use a traditional Windows Explorer left directory tree for the MVP. The left side is a Finder-style location sidebar, while hierarchy appears inside the main list.

## MVP Scope

The MVP includes:

- Win32 desktop window.
- Direct2D/DirectWrite custom-rendered UI.
- Finder-like sidebar with initial static sections:
  - Favorites
  - Locations
  - Tags
- Toolbar with visual buttons and a search box.
- Hierarchical file list:
  - Folder rows with disclosure arrows.
  - Expand/collapse.
  - Indented child rows.
  - Columns for name, modified date, size, and kind.
  - Full-row selection.
  - Vertical scrolling.
- Real filesystem browsing for a starting directory such as the current user's home directory.
- Basic file metadata:
  - Name
  - Directory/file kind
  - Modified timestamp
  - File size
- Bottom path bar.
- Basic filename search/filter over loaded/indexed nodes.

## Non-Goals For MVP

These are explicitly deferred:

- Zig index engine.
- Full-disk indexing.
- Thumbnail generation.
- Preview pane.
- Multiple tabs.
- Real tag persistence.
- Cloud drive integration.
- Complete context menu parity with Explorer.
- Drag and drop.
- File copy/move/delete workflows.
- Virtual desktop integration.
- Plugin architecture.

## Technology

Use a pure C++ stack for the MVP:

- Language: C++20 or newer.
- Build: CMake.
- Windowing: Win32 API.
- Rendering: Direct2D.
- Text: DirectWrite.
- File and path APIs: Win32 filesystem APIs plus selected Shell APIs where useful.
- Icons: start with lightweight file/folder icon rendering or Shell image list integration, depending on complexity during implementation.

The core reason for C++ only in the MVP is that the hardest first milestone is not raw search performance. It is native Windows UI, rendering, input, scrolling, filesystem integration, and Finder-like row behavior. Adding Zig before those boundaries are stable would increase complexity without solving the immediate risk.

## Architecture

The codebase should keep UI, filesystem state, rendering, and interaction separate.

### App Layer

Owns:

- WinMain.
- Main window creation.
- Message loop.
- DPI awareness setup.
- Application lifetime.

### Window Layer

Owns:

- Main HWND.
- Resize handling.
- Input dispatch.
- Render invalidation.
- High-level layout regions:
  - Sidebar
  - Toolbar
  - Header row
  - File list
  - Path bar

### Rendering Layer

Owns:

- Direct2D factory/device resources.
- DirectWrite factory/text formats.
- Brushes.
- Per-monitor DPI scaling.
- Shared primitives for text, rounded rectangles, separators, row backgrounds, and selection.

### File Model

Represents the directory hierarchy.

Each file node should store:

- Stable node id.
- Parent node id.
- Absolute path.
- Display name.
- Kind: directory or file.
- Expanded state.
- Loaded state.
- Modified time.
- File size.
- Child node ids.

The visible list is derived from the tree by flattening expanded nodes into ordered visible rows. This keeps rendering simple and fast.

### Filesystem Loader

Owns:

- Initial directory enumeration.
- Lazy child loading when a folder expands.
- Metadata conversion for display.
- Error handling for denied/inaccessible directories.

The MVP can load synchronously for small trees, but the architecture should allow a background loader later.

### List View

Owns:

- Visible row cache.
- Scroll offset.
- Hit testing.
- Selection.
- Expand/collapse.
- Column layout.
- Keyboard navigation.
- Mouse hover state.

The list must be custom-rendered. Standard Windows ListView/TreeView controls are not a good fit for matching Finder list behavior and visual density.

## Data Flow

1. App starts and creates the main window.
2. File model initializes with the chosen home/root directory.
3. Filesystem loader enumerates root children.
4. File model stores nodes in a tree.
5. List view asks model for the flattened visible rows.
6. Renderer draws only visible rows based on scroll position.
7. User expands a folder.
8. If children are not loaded, loader enumerates them.
9. File model updates expanded state and children.
10. List view rebuilds its visible row cache.
11. Window invalidates and redraws.

## UI Interaction

Required MVP interactions:

- Click row to select.
- Double-click file/folder:
  - Folder: navigate or expand, final behavior can be chosen during implementation.
  - File: optional for MVP; can be deferred.
- Click disclosure arrow to expand/collapse.
- Mouse wheel scroll.
- Keyboard up/down selection.
- Search box text filters by filename.

## Error Handling

Filesystem errors should not crash the UI.

Handle:

- Access denied.
- Deleted path during enumeration.
- Very long paths.
- Invalid or unavailable drives.
- Empty folders.

For MVP, inaccessible directories can appear as collapsed rows with no children and optional muted state.

## Testing And Verification

Initial verification should include:

- Build succeeds in Debug and Release.
- App opens a native window.
- Window resizes without broken layout.
- List renders at normal DPI and high DPI.
- Expanding/collapsing folders updates visible rows correctly.
- Selection remains stable after expansion and scrolling.
- Large folder with thousands of entries remains responsive enough for MVP.
- Search filters visible data correctly.

Automated tests can start with pure C++ unit tests for:

- Tree node insertion.
- Flattening expanded nodes.
- Search/filter behavior.
- Sort behavior once implemented.

Manual visual checks are required for the Finder-like surface.

## Development Phases

### Phase 1: Project Skeleton

- Initialize CMake C++ project.
- Add Win32 entry point.
- Create main window.
- Enable DPI awareness.
- Create Direct2D/DirectWrite resources.
- Paint a blank window.

### Phase 2: Static Finder Surface

- Draw sidebar.
- Draw toolbar.
- Draw table header.
- Draw static sample hierarchical rows.
- Draw path bar.
- Match the confirmed browser preview and reference screenshot.

### Phase 3: Interactive List

- Add visible row model.
- Add selection.
- Add disclosure arrow hit testing.
- Add expand/collapse.
- Add scrolling.
- Add keyboard navigation.

### Phase 4: Real Filesystem

- Enumerate real directory.
- Populate file tree.
- Lazy-load children on expand.
- Display modified time, size, and kind.

### Phase 5: Search And Polish

- Add search field input.
- Filter by filename.
- Preserve Finder-like visual polish.
- Improve icons.
- Add basic sorting if time permits.

## Deferred Zig Option

Zig remains a possible future choice for a dedicated indexing engine. It should only be reconsidered after the C++ MVP has stable interfaces and measurable indexing/search needs.

Possible future boundary:

```cpp
class FileIndex {
public:
    virtual NodeId root() const = 0;
    virtual std::span<const FileNode> children(NodeId id) = 0;
    virtual SearchResults search(std::wstring_view query) = 0;
};
```

If a later C++ implementation becomes hard to maintain or fails performance targets on very large indexes, a Zig implementation can replace the index backend behind a C ABI or another stable boundary.

## Open Decisions

- Whether double-clicking a folder expands inline or navigates into it.
- Whether the first implementation uses Shell system icons or simplified native-looking icons.
- Whether the first root is the user home directory or a configured development/test directory.
- Whether search initially filters the loaded tree only or performs background enumeration.

## Approved Direction

The confirmed direction is:

- Pure C++ MVP.
- Finder-like list view.
- Inline expandable hierarchy in the main list.
- No Zig in the first phase.
- No Explorer-style left directory tree.
