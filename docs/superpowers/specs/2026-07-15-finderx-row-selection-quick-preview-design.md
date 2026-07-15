# FinderX Row Selection and Quick Preview Design

## Goal

Fix three related file-surface interaction problems:

1. Clicking anywhere across an occupied file row selects that file, including the trailing columns and whitespace after a long name.
2. While Quick Preview is open, the underlying FinderX window cannot be operated with the mouse. Only Space or Escape closes the preview.
3. Text previews support selection, copying, mouse-wheel scrolling, and keyboard scrolling in a compact Finder-like presentation.

## Existing Causes

`FinderListView::hitTestRow` and `nodeAtPoint` already resolve the full width of an occupied row. `MainWindow::handleMessage`, however, treats points outside `isFileDragHotspot` as empty-space rubber-band starts. The drag hotspot is therefore incorrectly being used as the selection hit area.

Quick Preview is currently a Direct2D overlay drawn into the main window. The overlay does not participate in input routing, so mouse messages continue into the normal chrome and file-list handlers.

Text preview content is drawn as a single Direct2D text block. It has no selection model, caret, clipboard handling, or scroll position.

## Selected Approach

Keep the existing Direct2D preview shell and image preview path. For text content, place a native read-only multiline edit control over the preview body. This provides mature Windows text selection, clipboard, wheel, and keyboard behavior without reimplementing a text editor with DirectWrite hit testing.

Rejected alternatives:

- A custom DirectWrite text viewport would provide maximum visual control but would require custom selection, clipboard, scrolling, hit testing, and accessibility behavior.
- A separate top-level preview window would isolate input naturally but would substantially change the current preview architecture and window behavior.

## File Row Interaction

Selection and drag initiation use separate hit concepts:

- Any point inside an occupied row may select that row and participate in normal Ctrl/Shift selection.
- Only the existing icon/name drag hotspot may initiate a file drag.
- Only a point inside the list bounds that does not resolve to a row may begin rubber-band selection.
- Clicking a row outside the drag hotspot must not clear selection or start a rubber band.

The disclosure triangle retains its existing expansion behavior.

## Modal Preview Input

When Quick Preview is visible:

- Parent-window left-click, right-click, pointer-drag, context-menu, toolbar, sidebar, path-bar, and file-list mouse operations are consumed before normal input routing.
- Background clicks do not close the preview.
- Mouse input inside the text control remains available for selection and scrolling.
- Space and Escape close the preview.
- The text control subclasses keyboard handling so Space and Escape are forwarded to the preview close action even while the child control owns focus.
- Other text-navigation keys and Ctrl+C remain owned by the text control.
- Mouse-wheel input over the text control scrolls text; wheel input elsewhere on the overlay is consumed and does not scroll the file list.

Closing Quick Preview hides or destroys its native text control, clears preview-only state, returns focus to the main window, and repaints the file surface.

## Text Preview Presentation

The Direct2D shell continues to draw the dimmed background, compact header, filename, border, and preview surface using FinderX semantic theme tokens.

For text files:

- A read-only multiline edit control fills the inset body area.
- Vertical scrolling is enabled and horizontal overflow follows the existing compact preview width. Lines wrap to the viewport unless a later dedicated code-preview mode is introduced.
- The control uses the configured FinderX font size and theme-aware foreground/background colors.
- Native selection colors, caret-free read-only behavior, Ctrl+C, mouse selection, wheel scrolling, arrow keys, Home/End, and Page Up/Down are preserved.
- Empty files show `(Empty file)`.
- If loading was truncated, the existing `Preview truncated` status remains visible without covering selectable text.

The child control is repositioned on resize and recreated or restyled when preview content or theme changes.

## State and Responsibilities

`QuickPreviewContent` remains responsible only for loaded preview data.

`MainWindow` owns preview lifecycle and native control state:

- show/reload/close preview;
- create, position, style, and destroy the text control;
- route modal input;
- restore focus after close.

Pure helper functions should be introduced where practical for testable decisions such as row-click routing, modal input consumption, and preview geometry. Rendering remains in `QuickPreview.cpp`.

## Error Handling

- Existing file-loading errors continue to render inside the Direct2D preview shell.
- Failure to create the native text control falls back to a non-interactive error message instead of exposing the underlying file surface.
- Resizing to an unusably small preview area hides the text control but keeps modal input blocking active.
- Preview closure always clears child-window handles and subclass state safely.

## Testing

Add regression coverage before production changes:

- Clicking a row at a trailing-column coordinate resolves and selects the row.
- A trailing row click is not classified as rubber-band input.
- Empty list space is still classified as rubber-band input.
- Drag initiation remains limited to the file drag hotspot.
- Preview-visible input policy consumes background mouse and wheel input.
- Space and Escape close preview while ordinary text-navigation and Ctrl+C remain available to the text control.
- Text preview geometry produces a valid inset viewport.
- Existing text decoding, image loading, multi-selection, theme, and list interaction tests continue passing.

Verification includes the focused test executables, the full CTest suite, and a full application build. Manual checks cover light and dark themes, resize behavior, long text, selection/copy, wheel and keyboard scrolling, modal background blocking, and row selection across visible columns.

## Scope Boundaries

This change does not add rich-text syntax highlighting, find-in-preview, editable preview content, a separate preview window, or new preview file formats.
