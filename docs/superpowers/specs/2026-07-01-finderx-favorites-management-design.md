# FinderX Favorites Management Design

## Goal

Add practical favorite management to FinderX settings: rename favorites, remove favorites, and reorder favorites. Existing right-click add/remove behavior remains unchanged.

## Scope

This phase only manages favorite metadata already stored in `AppSettings::favorites`.

Included:

- Show favorites in the Settings dialog.
- Select one favorite at a time.
- Edit the selected favorite label.
- Remove the selected favorite.
- Move the selected favorite up or down.
- Save changes through the existing settings save path.
- Refresh sidebar after settings are accepted.

Excluded:

- Editing favorite paths in the Settings dialog.
- Drag-and-drop reordering.
- Custom favorite icons.
- Confirmation prompts for removal.

## User Experience

The Settings dialog will keep the current font size, icon size, and shortcut help controls. A new Favorites section will appear below the size inputs:

- A list box shows favorite labels.
- A read-only path field shows the selected favorite path.
- A label edit box changes the selected favorite label.
- Buttons: Up, Down, Remove.

The dialog applies favorite edits to an in-memory copy of settings. Pressing OK commits the edited settings. Pressing Cancel discards them.

If the user clears a favorite label, the dialog falls back to a label derived from the path's final component. If that is empty, it uses the full path.

## Architecture

`AppSettings` remains the source of truth for persisted favorites. Small helper functions will be added for favorite rename and reorder operations so behavior is testable outside the Win32 dialog.

`SettingsDialogValues` will continue to carry text values for font and icon sizes. Favorite edits will be applied directly to the dialog state's `resultSettings`, then `applySettingsDialogValues` will clamp size settings on OK.

The sidebar already reads `settings_.favorites` through `SidebarModel::refresh`, so no sidebar model changes are expected.

## Error Handling

- No selected favorite: Up, Down, Remove, and label edits do nothing.
- Move first favorite up: no-op.
- Move last favorite down: no-op.
- Remove selected favorite: remove it and select the next available favorite, or previous if removing the last item.
- Empty favorites list is allowed.

## Testing

Unit tests will cover new settings helpers:

- Rename favorite by index.
- Rename with empty label falls back from path.
- Move favorite up/down.
- Boundary moves are no-ops.
- Remove favorite by index and selection follow-up logic at helper level where practical.

Dialog helper tests will cover applying font/icon values without losing favorite edits.

Manual verification will cover Settings dialog interaction because the current Win32 dialog has no full UI automation harness.
