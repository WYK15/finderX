# FinderX Settings, Sorting, And Favorites Design

## Goal

Add the next usability layer for FinderX:

- a Finder-style toolbar entry for sorting and settings;
- sortable list columns for name, modified time, size, and kind;
- a left sidebar `Favorites` section;
- persisted global settings for font size, icon size, sort order, and favorites.

This design keeps the current Finder-like layout. It does not introduce a traditional Windows menu bar or a right-side settings drawer.

## Scope

In scope:

- Global application settings.
- Persistent settings file under `%APPDATA%\FinderX\settings.json`.
- Sort by:
  - name;
  - modified time;
  - size;
  - kind.
- Sort direction:
  - ascending;
  - descending.
- Sidebar `Favorites` section.
- Add current directory to favorites.
- Remove favorite entries from the sidebar.
- Font size and icon size settings.
- Immediate visual update after settings change.

Out of scope:

- Theme system.
- Per-folder sort memory.
- Per-tab settings.
- Drag-and-drop favorite reordering.
- Custom favorite icons.
- Complex settings pages.
- Import/export settings.

## User Experience

### Toolbar

FinderX keeps the current Finder-style toolbar. Add two controls:

- `Sort`
- `Settings`

`Sort` opens a compact popup menu:

- Name
- Modified
- Size
- Kind
- Ascending
- Descending

The list header also supports direct sorting by clicking columns. Clicking a different column sorts by that column. Clicking the same column toggles direction.

`Settings` opens a small modal dialog for:

- font size;
- icon size.

Settings changes apply immediately when confirmed.

### List Header Sorting

The table header remains:

- `名称`
- `修改日期`
- `大小`
- `种类`

The active sort column shows a small arrow:

- up arrow for ascending;
- down arrow for descending.

Default sort:

- folders first;
- name ascending.

Folder-first behavior remains active for all sort modes. Within folders and within files, the chosen column and direction apply.

### Sidebar

Replace the current flat sidebar with grouped sections:

```text
Favorites
  Home
  Desktop
  Documents
  Downloads
  user-added folders...

Locations
  This PC
  Applications
```

Home, Desktop, Documents, and Downloads become default favorites. They are not duplicated elsewhere in the sidebar.

Unavailable favorite paths are shown disabled. Clicking an unavailable favorite does nothing.

### Favorite Actions

Add current directory to favorites:

- available in the main list background context menu;
- label: `Add to Favorites`;
- adds the active tab's current real directory.

Remove a favorite:

- available from right-clicking a favorite sidebar item;
- label: `Remove from Favorites`.

Default favorites may be removed. A removed default favorite can be added again later by navigating to that directory and choosing `Add to Favorites`.

Duplicate favorites are not added. Path comparison is case-insensitive on Windows.

Virtual locations cannot be favorited:

- `This PC` cannot be added to favorites;
- drive roots can be added only after the user navigates into the drive root as a real directory.

## Architecture

### Settings Model

Create a settings module:

```text
src/settings/AppSettings.h
src/settings/AppSettings.cpp
```

Core data:

```cpp
enum class SortColumn {
    Name,
    Modified,
    Size,
    Kind
};

enum class SortDirection {
    Ascending,
    Descending
};

struct AppSettings {
    float fontSize = 13.0f;
    float iconSize = 14.0f;
    SortColumn sortColumn = SortColumn::Name;
    SortDirection sortDirection = SortDirection::Ascending;
    std::vector<FavoriteLocation> favorites;
};

struct FavoriteLocation {
    std::wstring label;
    std::wstring path;
};
```

Default favorites are built from the user's home directory:

- Home
- Desktop
- Documents
- Downloads

The settings module provides:

- default settings creation;
- load from disk;
- save to disk;
- add favorite;
- remove favorite;
- favorite de-duplication;
- clamping font and icon sizes to supported ranges.

Supported size ranges:

- font size: 11.0 to 18.0;
- icon size: 12.0 to 24.0.

### Settings Persistence

Store settings at:

```text
%APPDATA%\FinderX\settings.json
```

The file is UTF-8 JSON.

Example:

```json
{
  "fontSize": 13,
  "iconSize": 14,
  "sortColumn": "name",
  "sortDirection": "ascending",
  "favorites": [
    { "label": "Home", "path": "C:\\Users\\leo" },
    { "label": "Projects", "path": "D:\\Projects" }
  ]
}
```

Use a small fixed-shape reader/writer rather than adding a third-party JSON dependency. Invalid or missing fields fall back to defaults. A malformed file does not prevent FinderX from starting.

Save settings after:

- settings dialog confirmation;
- sort change;
- favorite add;
- favorite remove.

### Sorting

Keep `DirectoryLoader` focused on filesystem enumeration. Move user-facing ordering into a sorter:

```text
src/fs/FileSorter.h
src/fs/FileSorter.cpp
```

The sorter accepts:

- `std::vector<FileNode>&`;
- `SortColumn`;
- `SortDirection`.

Sorting rules:

1. folders before files;
2. compare selected column;
3. if selected values are equal, fall back to case-insensitive name ascending for stable discoverability.

Name and kind use case-insensitive string comparison.

Modified time needs a sortable value. Extend `FileNode` or add metadata during loading so sorting does not parse the localized display string. The preferred approach is to add a raw timestamp field to `FileNode`, such as:

```cpp
unsigned long long modifiedTicks = 0;
unsigned long long sizeBytes = 0;
```

Display strings remain unchanged:

- `modified`;
- `size`.

Size sorting uses `sizeBytes`. Directories sort by size as `0` within the folder group unless a later feature computes folder sizes.

### Sidebar Model

Extend `SidebarItem` with a group or role:

```cpp
enum class SidebarItemRole {
    Favorite,
    Location
};
```

`SidebarModel::refresh` should accept favorites from settings and build grouped rows.

The chrome drawing code renders section labels:

- `Favorites`
- `Locations`

This can be done without making section headers clickable.

### Settings Dialog

Create a small Win32 modal dialog or lightweight custom modal helper.

The first version only needs:

- font size numeric field or stepper;
- icon size numeric field or stepper;
- OK;
- Cancel.

Validation:

- non-numeric input falls back to current value;
- values are clamped to the supported ranges.

The dialog should not expose sort or favorites in the first version. Sorting is available from the toolbar/header, and favorites are managed through context menus.

### Render And Layout

`FinderListView` currently uses fixed constants:

- row height;
- icon size;
- text layout offsets.

Change it to accept a list view style object:

```cpp
struct ListViewStyle {
    float fontSize;
    float iconSize;
};
```

Derived values:

- row height = max(20, iconSize + 6, fontSize + 8);
- disclosure width remains stable enough for hit testing;
- icon/text spacing scales modestly with icon size.

`RenderContext` owns DirectWrite text formats. It should support recreating formats when font size changes.

Settings changes should:

1. update `AppSettings`;
2. update render/list style;
3. rebuild or invalidate visible UI;
4. save settings.

## Data Flow

Startup:

1. `MainWindow::create` loads settings.
2. If no settings file exists, defaults are created.
3. Sidebar is built from settings favorites.
4. Directory children are loaded.
5. Children are sorted using global sort settings.
6. UI renders with global font and icon size.

Sort change:

1. User clicks a header or Sort menu item.
2. `AppSettings.sortColumn` and `sortDirection` update.
3. Active directory tree is re-sorted or refreshed with the new order.
4. Settings are saved.
5. UI invalidates.

Favorite add:

1. User chooses `Add to Favorites`.
2. Active real directory path is added if not already present.
3. Label defaults to the directory name, or path for roots.
4. Settings are saved.
5. Sidebar refreshes.

Favorite remove:

1. User right-clicks a favorite sidebar row.
2. User chooses `Remove from Favorites`.
3. Favorite path is removed from settings.
4. Settings are saved.
5. Sidebar refreshes.

Settings change:

1. User opens Settings.
2. User changes font or icon size.
3. On OK, settings are clamped and stored.
4. Render text formats and list view style update.
5. Settings are saved.
6. UI invalidates.

## Error Handling

Settings load:

- If the file is missing, use defaults.
- If the file is malformed, use defaults and keep running.
- If a favorite path no longer exists, keep it in the settings file but show it disabled.

Settings save:

- Create `%APPDATA%\FinderX` if needed.
- If save fails, show a short status message: `Cannot save settings`.
- Keep the in-memory change even if saving fails.

Favorite add:

- If the current location is virtual, do not add it.
- If the path already exists in favorites, do not duplicate it.
- If the directory no longer exists, show `Cannot add favorite`.

Sort:

- If metadata is missing, sort that item after items with valid metadata for ascending and before for descending, within its folder/file group.
- If sorting cannot be applied for any reason, keep the current order and show no modal error.

## Testing

Add focused tests:

- settings defaults;
- settings JSON save/load round trip;
- malformed settings fallback;
- font/icon size clamping;
- favorite add/remove/de-duplicate;
- default favorites creation;
- sorter by name ascending/descending;
- sorter by modified ascending/descending;
- sorter by size ascending/descending;
- sorter by kind ascending/descending;
- folder-first behavior preserved under every sort column;
- sidebar grouping and selection;
- unavailable favorites shown disabled;
- list hit testing remains stable when icon size changes.

Manual smoke:

- Open FinderX and confirm Favorites/Locations sections render.
- Add current folder to favorites.
- Remove a favorite.
- Restart app and confirm favorites persist.
- Click each sortable header and confirm arrows/order.
- Open Settings, change font size and icon size, confirm UI updates.
- Restart app and confirm settings persist.

## Implementation Order

1. Add settings model and persistence tests.
2. Add sorter and sorting tests.
3. Add raw modified/size metadata to `FileNode` and `DirectoryLoader`.
4. Wire sorting into directory refresh/navigation.
5. Add sidebar favorites model and tests.
6. Add context menu actions for add/remove favorite.
7. Add toolbar Sort and header-click sorting.
8. Add settings dialog and font/icon size plumbing.
9. Run full Debug and Release verification.

## Open Decisions

No open product decisions remain for this batch.

Confirmed choices:

- Use Finder-style toolbar entries rather than a menu bar or right drawer.
- Use global settings, not per-tab or per-folder settings.
- Sort by name, modified time, size, and kind.
- Merge Home/Desktop/Documents/Downloads into Favorites rather than duplicate them.
- Persist settings under `%APPDATA%\FinderX\settings.json`.
