# FinderX Design Baseline

This document defines the visual and interaction baseline for future FinderX UI work. It uses SpaceUI (`spacedriveapp/spaceui`) as the reference system, adapted for FinderX's native C++/Win32/Direct2D implementation.

## Source Reference

SpaceUI is a shared UI design system for Spacedrive applications. The parts that matter most for FinderX are:

- `@spacedrive/tokens`: semantic color, radius, font, and theme tokens.
- `@spacedrive/primitives`: reusable controls such as buttons, search bars, dialogs, context menus, tooltips, tabs, sliders, switches, and resizable surfaces.
- `@spacedrive/explorer`: file-manager-oriented UI components such as file thumbnails, grid items, rename inputs, path bars, lists, inspectors, drag overlays, and quick preview patterns.
- `@spacedrive/icons`: file type icons, extension badge icons, brand icons, and icon resolution utilities.

FinderX should borrow the design language and component behavior, not the React/Tailwind implementation model.

## Design Personality

FinderX should feel like a polished local file manager: quiet, compact, fast, and visually refined. The UI should prioritize repeated file operations over marketing-style presentation.

Use these principles:

- Dense but readable layout.
- Low visual noise: subtle borders, restrained backgrounds, and clear selection states.
- Icon-first commands with tooltips where labels are not visible.
- A strong file browsing surface, with supporting controls kept compact.
- Smooth feedback for hover, selection, drag, rename, context menus, dialogs, and search focus.
- Light and dark themes should share the same semantic structure instead of separate one-off colors.

## Token Model

Future UI colors should be organized by purpose, not by raw color names. Prefer names like:

- `accent`, `accentFaint`, `accentDeep`
- `ink`, `inkDull`, `inkFaint`
- `app`, `appBox`, `appOverlay`, `appInput`, `appLine`
- `appHover`, `appSelected`, `appActive`
- `sidebar`, `sidebarBox`, `sidebarLine`, `sidebarSelected`
- `menu`, `menuLine`, `menuHover`, `menuSelected`
- `statusSuccess`, `statusWarning`, `statusError`, `statusInfo`

SpaceUI's default dark palette is a cool neutral base with a blue accent. FinderX should keep that direction, while preserving Windows readability and contrast.

Recommended defaults:

- Accent: Windows-friendly blue.
- Text: primary, secondary, and faint text levels.
- Surfaces: app background, raised box, overlay, input, hover, selected, and active.
- Borders: thin, low-contrast separators rather than heavy outlines.
- Radius: 6px for ordinary controls, 8px for larger panels, 10px maximum for windows/dialog shells.

Avoid one-off hard-coded colors when adding new UI. Add or reuse a semantic theme value first.

## Theme Switching

FinderX must support both dark and light themes as first-class UI modes.

Theme behavior:

- The current theme should be user-configurable in Settings.
- Theme selection should persist between app launches.
- If a system-following option is added later, it should map Windows light/dark preference into the same semantic theme tokens.
- Switching themes should update the whole app consistently: title bar, toolbar, sidebar, file list/tree, path bar, menus, dialogs, settings, drag overlay, icons, and focus states.
- Avoid partial theme changes where one surface updates but adjacent controls keep the old palette.

Design requirements:

- Every new color must be represented as a semantic token with both dark and light values.
- Do not hard-code dark-only or light-only colors inside rendering code.
- Text, icons, row selection, column separators, input carets, resize handles, and context menu highlights must remain readable in both themes.
- Disabled and secondary text should be visibly muted but not too low-contrast.
- Accent blue should remain recognizable in both themes, with adjusted brightness if needed.

Icon requirements:

- Prefer theme-aware icons when using rich file/kind assets.
- Monochrome command icons should draw from current theme ink/accent colors.
- Imported SpaceUI icons with `_Light` variants should be resolved through an icon resolver instead of selected ad hoc in view code.

## Typography

Use the configured FinderX font setting as the source of truth. The style baseline should remain compact:

- File rows and sidebar items: small but readable.
- Toolbar controls and path segments: compact text.
- Dialog titles: only slightly larger than body text.
- Do not use hero-scale typography inside the application.

Text must fit its container at all supported font sizes. Prefer truncation with tooltips for file names and paths where full text cannot fit.

## Layout

FinderX's primary layout should remain:

- Native themed title bar.
- Compact toolbar.
- Left sidebar for locations, favorites, drives, and navigation groups.
- Main file tree/list surface.
- Bottom path bar with editable path behavior.

Do not add card-heavy page sections. Use panels only when they represent a functional surface such as settings, inspector, preview, or a modal.

Spacing should be tight and consistent. Prefer 4px, 6px, 8px, 12px, and 16px increments.

## Toolbar

The toolbar should follow SpaceUI's circular/segmented top-bar direction:

- Icon-first actions.
- Compact hit targets.
- Rounded controls with subtle borders.
- Group related actions with separators.
- Show tooltips for icon-only commands.
- Use active/accent styling only for selected modes or primary actions.

Toolbar customization should keep the same visual language. New commands should be added as toolbar actions first when they are frequent operations, and context menu entries second when they are situational.

## Sidebar

The sidebar should be calm and navigational:

- Group labels should be faint and compact.
- Items should use small icons plus labels.
- Selected rows should use a filled subtle selection state, not a heavy border.
- Hover states should be visible but quiet.
- Favorites, This PC, drives, and common folders should use consistent row height and icon alignment.

## File List And Tree

The file surface is the core product experience.

List/tree requirements:

- Keep rows compact and stable in height.
- Preserve clear column headers and draggable separators.
- Use subtle alternating/hover/selected states.
- Selected rows should have a clear accent or selected background.
- Expansion arrows should appear only for folders with children.
- Refresh or external filesystem changes must preserve expanded state when possible.
- Inline rename should feel native: auto focus, selected text, Enter confirms, Escape cancels.

Sorting should be clear from the column header and should not disturb selection more than necessary.

## Path Bar

The bottom path bar should support both breadcrumb navigation and editable path behavior:

- Breadcrumb segments should size to content when possible.
- Long paths should truncate gracefully without hiding interaction.
- Clicking a segment navigates to that level.
- Editing mode should show a correctly aligned caret, support selection, arrow navigation, Ctrl+A, copy, paste, and normal text interaction.

## Search

Search should follow SpaceUI's compact search bar pattern:

- Rounded field.
- Search icon at the leading edge.
- Clear button when text exists.
- Visible caret and focus state.
- Background changes subtly on hover/focus.

Search should never look like a decorative hero input. It is a utility control.

## Menus And Dialogs

Context menus should use:

- Compact row height.
- Optional leading icon.
- Right-aligned shortcut text.
- Separators between groups.
- Accent highlight for hovered item.
- Danger styling only for destructive actions.

Dialogs/settings should use:

- Functional grouping.
- Compact controls.
- Clear primary/cancel buttons.
- Subtle panel border.
- No nested decorative cards.

## Motion And Feedback

Animations should be short and purposeful:

- Hover and active states: about 100ms.
- Dialog/menu entrance: subtle fade/slide.
- Drag operations: show an overlay or cursor-adjacent label so the user knows what is being dragged and where it can be dropped.
- Avoid decorative animation unrelated to file management.

## Icon Strategy

SpaceUI's `@spacedrive/icons` package is a useful future source for FinderX icons. It includes:

- General kind icons such as folder, document, archive, image, audio, video, executable, drive, home, cloud, database, and code.
- Light and dark variants for many file/kind icons.
- 20px variants for compact list usage.
- Extension-specific SVG icons and an `icons.json` mapping for file names, file extensions, and language IDs.
- Utility functions that resolve an icon from kind, extension, dark mode, and directory state.

Recommended FinderX adoption path:

1. Use SpaceUI icons as visual reference for style matching.
2. If importing assets directly, copy only the needed icons first and include MIT license attribution.
3. Store imported assets under a dedicated asset folder such as `resources/icons/spacedrive/`.
4. Add a small native icon resolver in FinderX that maps file kind, extension, directory state, and theme to an icon asset.
5. Prefer 20px or compact variants for list/tree rows, larger variants only for grid/preview modes.
6. Keep toolbar/action icons visually separate from file-type icons. Use monochrome vector-style icons for commands, and richer file/kind icons for files.

For current C++/Direct2D rendering, imported SVG/PNG assets should be converted or loaded through a deliberate resource pipeline. Do not scatter raw asset loading across view code.

## Implementation Rules

When adding or changing UI:

- Read this document first.
- Reuse existing FinderX theme settings and extend them semantically.
- Keep behavior and rendering separate where practical.
- Avoid introducing new color constants inside control rendering.
- Prefer reusable helper functions for repeated controls such as toolbar buttons, menu rows, path segments, and icon drawing.
- Verify light theme and dark theme.
- Verify live theme switching if the changed surface stays mounted while settings change.
- Verify default, hover, active, focused, disabled, selected, and dragging states when applicable.
- Verify text fit at the minimum and maximum configured font size.

## UI Change Checklist

Before finishing a UI change, check:

- Does it match the compact SpaceUI-inspired FinderX baseline?
- Does it work in both light and dark themes?
- Are icons aligned and sized consistently?
- Are focus, hover, selection, and disabled states visible?
- Does it avoid decorative cards or oversized type?
- Does it preserve FinderX's file-manager ergonomics?
- Are imported third-party assets attributed if copied into the repo?
