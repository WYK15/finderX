# FinderX Finder Visual Restoration Design

## Goal

Make FinderX visually closer to the provided macOS Finder list-view screenshot while preserving the current navigation behavior, filesystem loading, and lazy tree expansion.

## Scope

This stage is visual only. It improves the appearance of the existing Win32/Direct2D interface without adding new file-management features.

Included:

- Replace `[D]` and `[F]` text markers with small drawn folder and file icons.
- Add small drawn sidebar icons for Applications, Documents, Desktop, Home, and Downloads.
- Refine sidebar spacing, selection, typography, and disabled state.
- Refine toolbar Back/Forward controls, title placement, view-mode glyphs, and search field.
- Render the bottom path bar as Finder-style path segments separated by chevrons.
- Tune list row height, indentation, alternating backgrounds, column headers, selection colors, and text colors.

Excluded:

- Search behavior.
- Context menus.
- Delete, rename, copy, or other file operations.
- Image resource loading.
- Icon theme packs.
- Clickable path bar segments.
- Animation.
- Full DPI strategy beyond preserving responsive bounds.

## Visual Direction

The selected direction is "Finder Screenshot Match." FinderX should continue to look like a native Windows app technically, but its surface should visually follow the screenshot:

- pale gray sidebar
- compact toolbar
- tight list rows
- blue selected row
- muted alternating row backgrounds
- bottom path bar
- small iconography rather than text markers

The UI should not pivot toward Windows Explorer styling or a modern card/grid browser in this stage.

## Architecture

Keep visual changes inside UI rendering classes:

- `RenderContext` gains only small drawing primitives if needed.
- `FinderChrome` owns chrome-level drawing: sidebar, toolbar, header, path bar.
- `FinderListView` owns list-row drawing: disclosure, file/folder icon, row text, row backgrounds.
- Navigation, filesystem loading, and model state remain unchanged.

Do not add external image files. Icons should be drawn with Direct2D primitives and short text glyphs only where primitive drawing is not enough.

## Components

### RenderContext

The current rendering API supports text, lines, rectangles, and rounded rectangles. If icon drawing needs outlines, add a minimal `drawRoundedRect` helper. Avoid broad rendering abstractions.

### FinderListView

List rows should replace:

- `> [D] folder`
- `  [F] file`

with:

- disclosure triangle
- small folder or file icon
- name text

Folder icons are small yellow/blue-tinted folder shapes drawn with rectangles and rounded rectangles. File icons are small white document shapes with a muted outline and folded-corner hint if practical.

List indentation stays based on `VisibleRow.depth`. Disclosure hit testing must remain stable after visual spacing changes.

### FinderChrome

Chrome rendering should be polished but still compact:

- Sidebar rows use icon + label alignment.
- Sidebar selected item remains a subtle rounded gray row.
- Toolbar Back/Forward are separated glyph buttons with disabled state.
- View-mode glyphs remain visual only and non-interactive.
- Search field stays visual only.
- Header labels retain responsive hiding behavior when width is narrow.
- Path bar splits `ChromeState::pathText` into segments and draws compact separators.

### State And Data

No model changes are required. Existing `ChromeState`, `SidebarItem`, `FileNode`, and `VisibleRow` provide enough information.

The path bar can derive segments from `ChromeState::pathText` by splitting on `\\` and `/`. Drive roots such as `C:` should be preserved as a segment. If `statusText` is non-empty, path segments are not drawn; the status text is drawn instead.

## Interaction Behavior

Visual changes must not alter current behavior:

- Sidebar clicking still navigates available items.
- Toolbar Back/Forward hit testing still works.
- Disclosure arrows still expand/collapse folders.
- Double-click and Enter activation still work.
- Backspace navigation still works.
- Wheel scrolling and Up/Down selection still work.

Hit-test rectangles should remain aligned with visible controls after spacing changes.

## Error Handling

Visual restoration should not change navigation or filesystem error handling.

When `ChromeState::statusText` is set, the path bar should continue to show the error/status text in a red-ish color instead of path segments.

## Testing And Verification

Automated tests remain focused on existing non-visual behavior:

- `finderx_model_tests`
- `finderx_fs_tests`
- `finderx_navigation_tests`

Visual changes require manual app checks:

- FinderX starts and renders non-empty chrome.
- Folder rows show drawn folder icons instead of `[D]`.
- File rows show drawn file icons instead of `[F]`.
- Sidebar rows show small icons.
- Path bar shows segmented path when no status is active.
- Status text still appears when an error is set.
- Text does not overlap at the default 1188x768 window size.
- Narrow widths do not produce obvious overlapping column text.
- Existing navigation and selection behavior still works.

## Acceptance Criteria

- No `[D]` or `[F]` markers appear in list row names.
- Sidebar items include visible icons.
- Toolbar and path bar look closer to the Finder screenshot direction.
- List rows remain compact and readable.
- Existing navigation, lazy expansion, and activation behavior still work.
- Debug and Release `FinderX` builds succeed.
- Existing tests pass.
