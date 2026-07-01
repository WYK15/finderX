# FinderX Next Features Design

## Scope

This design covers the next six requested feature areas:

- Address/path input.
- Better file operation feedback.
- Real Windows shell icons.
- Preview panel.
- Favorite management.
- Large-directory performance.

These are intentionally split into phases because they touch different risk areas: chrome input, shell operations, rendering, settings, and list performance.

## Phase 1: Address Input

The first deliverable adds path entry without changing the file tree model. FinderX will expose an address input mode from the existing chrome:

- Clicking the path bar blank area, or using a later keyboard shortcut, focuses a text field containing the current path.
- Enter navigates to the typed path using the existing `navigateToLocation` flow.
- Esc cancels address editing and restores the existing chrome state.
- Missing or invalid paths keep the user in the current location and show the existing status text.

The first implementation can reuse `ChromeState` and `FinderChrome::hitTest`. Input state remains per tab, like search text.

## Phase 2: File Operation Feedback And Favorites

File operation feedback should remain compatible with the current shell-backed operations. FinderX will improve status messaging around copy, move, delete, rename, and create operations, without replacing the Windows shell progress UI yet.

Favorite management should extend the existing `AppSettings::favorites` storage:

- Rename favorites from settings.
- Remove favorites from settings.
- Move favorites up/down in settings.
- Keep right-click add/remove behavior.

## Phase 3: Shell Icons And Preview Panel

Real icons should come from Windows Shell APIs and be cached by extension/type to avoid expensive per-row calls during drawing. The first implementation should fall back to the current drawn icons if shell icon extraction fails.

Preview should start as a non-invasive panel:

- Basic metadata for any file.
- Text preview for small text files.
- Image preview for common image formats when decoding succeeds.
- Folder preview shows item count when cheap to compute.

The panel should be optional or toggleable so dense list workflows stay efficient.

## Phase 4: Large Directory Performance

Performance work should be done after UI behavior stabilizes:

- Virtualize visible row drawing and hit testing.
- Avoid recomputing flattened rows more often than necessary.
- Add loading state for slow directory reads.
- Consider async directory loading only after the synchronous boundaries are explicit and tested.

## Non-Goals

- Full indexed search is not part of these phases.
- Custom embedded copy progress is not part of phase 2.
- Replacing the file model is not part of phase 1.

## Testing Strategy

Each phase gets targeted unit tests first:

- Chrome hit testing and input state for address entry.
- Settings/favorites mutation tests.
- Icon cache tests around fallback behavior where practical.
- Preview model tests without requiring a live UI.
- List view tests for virtualization math and selection preservation.

Release builds and focused CTest runs are required before packaging.
