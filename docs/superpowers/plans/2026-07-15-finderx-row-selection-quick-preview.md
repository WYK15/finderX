# FinderX Row Selection and Quick Preview Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make every occupied file row selectable across its full width and make Quick Preview modal while providing a native selectable, copyable, scrollable text viewport.

**Architecture:** Separate row selection hit testing from drag-hotspot testing, then centralize preview geometry and modal-input decisions in testable Quick Preview helpers. Keep the Direct2D preview shell and image renderer, while `MainWindow` owns a read-only multiline Win32 edit child for text content and its lifecycle.

**Tech Stack:** C++20, Win32 window/message APIs, Direct2D/DirectWrite, CMake/CTest, existing FinderX theme and DPI helpers.

## Global Constraints

- Preserve all unrelated dirty-worktree changes; do not reset, overwrite, or stage unrelated hunks.
- Clicking an occupied row anywhere across its width selects it; only actual empty list space starts rubber-band selection.
- Keep drag initiation limited to the existing icon/name drag hotspot.
- While Quick Preview is visible, background mouse input is consumed and does not close the preview.
- Only Space or Escape closes Quick Preview.
- Text preview supports native selection, Ctrl+C, mouse-wheel scrolling, and keyboard scrolling.
- Use FinderX semantic theme tokens in both light and dark modes and the configured FinderX font.
- Do not add syntax highlighting, preview editing, new preview formats, or a separate preview window.
- Because `MainWindow.cpp`, `MainWindow.h`, `CMakeLists.txt`, and Quick Preview files already contain user-owned uncommitted work, do not create implementation commits that would capture those pre-existing changes. Verify with exact diffs instead.

---

## File Structure

- Modify `src/ui/FinderListView.h`: declare pure list-pointer routing helpers.
- Modify `src/ui/FinderListView.cpp`: implement row-versus-empty and drag-eligibility decisions.
- Modify `src/ui/FinderListViewTests.cpp`: cover trailing-row selection, empty-space rubber-band routing, and hotspot-only dragging.
- Modify `src/ui/QuickPreview.h`: expose preview layout, modal mouse-message, and close-key helpers.
- Modify `src/ui/QuickPreview.cpp`: centralize geometry and reuse it for Direct2D rendering.
- Modify `src/ui/QuickPreviewTests.cpp`: cover modal routing, close keys, and valid/invalid preview geometry.
- Modify `src/ui/MainWindow.h`: declare text-control lifecycle/subclass methods and native resource members.
- Modify `src/ui/MainWindow.cpp`: use full-row selection routing, block background preview input, and manage the read-only text control.
- Keep `CMakeLists.txt` unchanged unless the existing untracked Quick Preview test target fails to include a newly required source or Windows library.

---

### Task 1: Separate Full-Row Selection From Drag Hotspots

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`
- Test: `src/ui/FinderListViewTests.cpp`
- Modify: `src/ui/MainWindow.cpp`

**Interfaces:**
- Produces: `bool shouldStartListRubberBand(NodeId nodeAtPoint)`.
- Produces: `bool canStartFileDrag(NodeId nodeAtPoint, bool fileDragHotspot)`.
- Consumes: existing `FinderListView::nodeAtPoint`, `FinderListView::isFileDragHotspot`, and `FinderListView::onMouseDown`.

- [ ] **Step 1: Write failing list-pointer routing tests**

Add tests beside the existing drag-hotspot block in `FinderListViewTests.cpp`:

```cpp
require(!shouldStartListRubberBand(rows[2].nodeId),
        "an occupied row must not start rubber-band selection");
require(shouldStartListRubberBand(kInvalidNodeId),
        "empty list space should start rubber-band selection");
require(canStartFileDrag(rows[2].nodeId, true),
        "a row drag hotspot should allow drag initiation");
require(!canStartFileDrag(rows[2].nodeId, false),
        "trailing row space should not initiate a drag");

const ListInteractionResult trailingClick =
    view.onMouseDown(520.0f, rowY(2), bounds, false, false);
require(trailingClick.changed,
        "clicking the trailing part of an occupied row should change selection");
require(view.selectedNode() == rows[2].nodeId,
        "trailing row click should select that row");
```

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```powershell
.\scripts\build.ps1 -Target finderx_list_view_tests
```

Expected: compilation fails because `shouldStartListRubberBand` and `canStartFileDrag` are not declared.

- [ ] **Step 3: Add the minimal routing helpers**

Declare in `FinderListView.h` in namespace `finderx`:

```cpp
bool shouldStartListRubberBand(NodeId nodeAtPoint);
bool canStartFileDrag(NodeId nodeAtPoint, bool fileDragHotspot);
```

Implement in `FinderListView.cpp`:

```cpp
bool shouldStartListRubberBand(NodeId nodeAtPoint) {
    return nodeAtPoint == kInvalidNodeId;
}

bool canStartFileDrag(NodeId nodeAtPoint, bool fileDragHotspot) {
    return nodeAtPoint != kInvalidNodeId && fileDragHotspot;
}
```

- [ ] **Step 4: Route `WM_LBUTTONDOWN` with the new decisions**

In `MainWindow::handleMessage`, replace the rubber-band condition with:

```cpp
if (shouldStartListRubberBand(mouseDownNode) && containsPoint(rects.list, dipPoint)) {
    // Keep the existing rubber-band setup body unchanged.
}
```

Keep calling `FinderListView::onMouseDown` for every occupied row. Gate both the selected-item deferred-drag branch and the post-selection `PendingMove` branch with:

```cpp
canStartFileDrag(mouseDownNode, fileDragHotspot)
```

This ensures a trailing click selects without starting a drag.

- [ ] **Step 5: Run the focused test and verify GREEN**

Run:

```powershell
.\scripts\build.ps1 -Target finderx_list_view_tests
.\build\Debug\finderx_list_view_tests.exe
```

Expected: build exits 0 and prints `FinderListViewTests passed`.

- [ ] **Step 6: Inspect the exact diff**

Run:

```powershell
git diff --check -- src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp src/ui/MainWindow.cpp
git diff -- src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp src/ui/MainWindow.cpp
```

Expected: no whitespace errors; only the routing helpers, tests, and two `WM_LBUTTONDOWN` decisions are new for this task.

---

### Task 2: Centralize Preview Geometry and Modal Input Policy

**Files:**
- Modify: `src/ui/QuickPreview.h`
- Modify: `src/ui/QuickPreview.cpp`
- Test: `src/ui/QuickPreviewTests.cpp`

**Interfaces:**
- Produces: `QuickPreviewLayout quickPreviewLayout(D2D1_SIZE_F size)`.
- Produces: `bool isQuickPreviewModalMouseMessage(UINT message)`.
- Produces: `bool isQuickPreviewCloseKey(WPARAM key)`.
- Consumes: `D2D1_SIZE_F`, `D2D1_RECT_F`, Win32 message constants, and the existing Direct2D renderer.

- [ ] **Step 1: Write failing helper tests**

Append to `QuickPreviewTests.cpp` before its success message:

```cpp
// Add #include <d2d1helper.h> with the other includes.
const ui::QuickPreviewLayout layout = ui::quickPreviewLayout(D2D1::SizeF(1200.0f, 800.0f));
require(layout.visible, "large windows should produce a visible preview layout");
require(layout.shell.left < layout.shell.right && layout.shell.top < layout.shell.bottom,
        "preview shell should have positive area");
require(layout.textViewport.left > layout.body.left
        && layout.textViewport.top > layout.body.top
        && layout.textViewport.right < layout.body.right
        && layout.textViewport.bottom < layout.body.bottom,
        "text viewport should be inset inside the preview body");
require(!ui::quickPreviewLayout(D2D1::SizeF(200.0f, 140.0f)).visible,
        "undersized windows should hide preview content geometry");

require(ui::isQuickPreviewModalMouseMessage(WM_LBUTTONDOWN),
        "preview should consume background left clicks");
require(ui::isQuickPreviewModalMouseMessage(WM_RBUTTONDOWN),
        "preview should consume background right clicks");
require(ui::isQuickPreviewModalMouseMessage(WM_MOUSEWHEEL),
        "preview should consume background wheel input");
require(!ui::isQuickPreviewModalMouseMessage(WM_PAINT),
        "preview should not consume paint messages");
require(ui::isQuickPreviewCloseKey(VK_SPACE), "Space should close preview");
require(ui::isQuickPreviewCloseKey(VK_ESCAPE), "Escape should close preview");
require(!ui::isQuickPreviewCloseKey(VK_DOWN), "text navigation should remain available");
```

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```powershell
.\scripts\build.ps1 -Target finderx_quick_preview_tests
```

Expected: compilation fails because the layout and input-policy APIs do not exist.

- [ ] **Step 3: Add layout and input-policy declarations**

Add to `QuickPreview.h`:

```cpp
struct QuickPreviewLayout {
    bool visible = false;
    D2D1_RECT_F shell{};
    D2D1_RECT_F header{};
    D2D1_RECT_F body{};
    D2D1_RECT_F textViewport{};
};

QuickPreviewLayout quickPreviewLayout(D2D1_SIZE_F size);
bool isQuickPreviewModalMouseMessage(UINT message);
bool isQuickPreviewCloseKey(WPARAM key);
```

- [ ] **Step 4: Implement minimal geometry and policy helpers**

Move the existing preview constants and rectangle calculation from `drawQuickPreview` into `quickPreviewLayout`. Use the existing `52.0f` margin, `760.0f` maximum width, `560.0f` maximum height, `44.0f` header height, and `18.0f` body inset. Define `textViewport` as the body inset by 14 DIPs horizontally and 12 DIPs vertically. Set `visible` only when width is greater than `220.0f` and height is greater than `160.0f`.

Implement the policies with explicit switches:

```cpp
bool isQuickPreviewModalMouseMessage(UINT message) {
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_CONTEXTMENU:
        return true;
    default:
        return false;
    }
}

bool isQuickPreviewCloseKey(WPARAM key) {
    return key == VK_SPACE || key == VK_ESCAPE;
}
```

- [ ] **Step 5: Reuse `QuickPreviewLayout` in rendering**

At the beginning of `drawQuickPreview`:

```cpp
const D2D1_SIZE_F size = render.sizeDips();
const QuickPreviewLayout layout = quickPreviewLayout(size);
if (!layout.visible) {
    return;
}
const D2D1_RECT_F& shell = layout.shell;
const D2D1_RECT_F& header = layout.header;
const D2D1_RECT_F& body = layout.body;
```

For text content, draw only the body surface/border and truncation status. Do not draw `content.text` when the native viewport will be present. Keep image and error rendering unchanged. When `content.message` is non-empty, `layoutQuickPreviewTextControl` must reduce the edit rectangle's bottom by 28 DIPs so the status remains visible and never sits underneath the child edit.

- [ ] **Step 6: Run the focused test and verify GREEN**

Run:

```powershell
.\scripts\build.ps1 -Target finderx_quick_preview_tests
.\build\Debug\finderx_quick_preview_tests.exe
```

Expected: build exits 0 and prints `QuickPreviewTests passed`.

---

### Task 3: Add the Native Read-Only Text Preview Viewport

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify only if required by a linker error: `CMakeLists.txt`

**Interfaces:**
- Consumes: `ui::quickPreviewLayout`, `ui::isQuickPreviewModalMouseMessage`, `ui::isQuickPreviewCloseKey`, `QuickPreviewContent`, `themeTokens`, `dipsToPixels`, and `RenderContext::dpiScale`.
- Produces private methods: `syncQuickPreviewTextControl`, `layoutQuickPreviewTextControl`, `destroyQuickPreviewTextControl`, and `QuickPreviewTextEditProc`.

- [ ] **Step 1: Add lifecycle declarations and native resource members**

Add to `MainWindow.h`:

```cpp
static LRESULT CALLBACK QuickPreviewTextEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void syncQuickPreviewTextControl();
void layoutQuickPreviewTextControl();
void destroyQuickPreviewTextControl();
```

Add members adjacent to the existing preview state:

```cpp
HWND quickPreviewTextEdit_ = nullptr;
WNDPROC quickPreviewTextOriginalProc_ = nullptr;
HFONT quickPreviewTextFont_ = nullptr;
HBRUSH quickPreviewTextBackgroundBrush_ = nullptr;
COLORREF quickPreviewTextColor_ = RGB(0, 0, 0);
```

- [ ] **Step 2: Implement the text-control subclass**

Implement `QuickPreviewTextEditProc` using `GWLP_USERDATA`. On `WM_KEYDOWN`, if `ui::isQuickPreviewCloseKey(wParam)` is true, call `closeQuickPreview()` and return 0. Otherwise call the saved original edit procedure so Ctrl+C, selection, arrows, Home/End, Page Up/Down, and wheel behavior remain native.

```cpp
LRESULT CALLBACK MainWindow::QuickPreviewTextEditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (window && message == WM_KEYDOWN && ui::isQuickPreviewCloseKey(wParam)) {
        window->closeQuickPreview();
        return 0;
    }
    if (window && window->quickPreviewTextOriginalProc_) {
        return CallWindowProcW(window->quickPreviewTextOriginalProc_, hwnd, message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
```

- [ ] **Step 3: Implement creation, styling, positioning, and destruction**

`syncQuickPreviewTextControl` must:

1. Call `destroyQuickPreviewTextControl` unless the preview is visible and `quickPreviewContent_.kind == ui::QuickPreviewKind::Text`.
2. Create a child `EDIT` with `WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY` and `WS_EX_CLIENTEDGE` when no control exists.
3. Store `this` with `GWLP_USERDATA`, subclass with `SetWindowLongPtrW(GWLP_WNDPROC, ...)`, and set margins using `EM_SETMARGINS`.
4. Set text to `L"(Empty file)"` for empty content or `quickPreviewContent_.text.c_str()` otherwise.
5. Recreate its `HFONT` from `settings_.fontFamily`, `settings_.fontSize`, and the current DPI; apply with `WM_SETFONT`.
6. Recreate the background brush and text color from `themeTokens(settings_.themeMode).appBox` and `.ink`.
7. Call `layoutQuickPreviewTextControl`, show the control only for valid geometry, and focus it.

If `CreateWindowExW` fails, replace the text preview data with `QuickPreviewKind::Error` and the message `L"Cannot create text preview"`, leave `quickPreviewVisible_` true, and repaint. The parent modal-input guard must remain active so control-creation failure never exposes background interaction.

`layoutQuickPreviewTextControl` must convert `ui::quickPreviewLayout(render_.sizeDips()).textViewport` through the existing `dipsToPixels` helper and call `SetWindowPos(..., SWP_NOZORDER | SWP_NOACTIVATE)`.

`destroyQuickPreviewTextControl` must restore the original WNDPROC if the child still exists, destroy the window, delete the font and brush, and null every associated handle.

- [ ] **Step 4: Add theme-aware edit coloring**

Extend the existing `WM_CTLCOLOREDIT` branch:

```cpp
if (reinterpret_cast<HWND>(lParam) == quickPreviewTextEdit_
    && quickPreviewTextBackgroundBrush_) {
    SetTextColor(reinterpret_cast<HDC>(wParam), quickPreviewTextColor_);
    SetBkColor(reinterpret_cast<HDC>(wParam),
               colorRef(themeTokens(settings_.themeMode).appBox));
    return reinterpret_cast<LRESULT>(quickPreviewTextBackgroundBrush_);
}
```

- [ ] **Step 5: Wire preview lifecycle and resizing**

- After loading content in `toggleQuickPreview` and `reloadQuickPreviewForSelection`, call `syncQuickPreviewTextControl`.
- At the start of `closeQuickPreview`, call `destroyQuickPreviewTextControl`; after clearing preview state call `SetFocus(hwnd_)`.
- In `WM_SIZE`, after resizing the render target, call `layoutQuickPreviewTextControl` when preview is visible.
- In `applyVisualSettings`, call `syncQuickPreviewTextControl` when preview is visible so live theme/font changes update the control.
- In `WM_DESTROY`, call `destroyQuickPreviewTextControl` before destroying other native resources.

- [ ] **Step 6: Make the preview overlay modal to parent mouse input**

At the top of `MainWindow::handleMessage`, before the main switch, add:

```cpp
if (quickPreviewVisible_ && ui::isQuickPreviewModalMouseMessage(message)) {
    return 0;
}
```

Child edit mouse messages go directly to the edit control, so this blocks only background interaction. Update the parent `WM_KEYDOWN` preview branch to use `ui::isQuickPreviewCloseKey(wParam)`.

- [ ] **Step 7: Build the application**

Run:

```powershell
.\scripts\build.ps1 -Target FinderX
```

Expected: build exits 0. If the linker cannot write `FinderX.exe`, close the running FinderX process and rerun; do not change code for a locked output file.

- [ ] **Step 8: Manually verify the native interaction boundary**

Launch `build\Debug\FinderX.exe` and check:

1. Click the name, date, size, kind, and trailing area of several rows; each selects the intended row.
2. Dragging still begins only from the icon/name hotspot.
3. Open a multiline TXT file with Space.
4. Select text with the mouse and copy with Ctrl+C.
5. Scroll with the wheel, arrow keys, Home/End, and Page Up/Down.
6. Click the dimmed file list, sidebar, toolbar, and path bar; none responds and the preview remains open.
7. Close with Space, reopen, then close with Escape.
8. Resize the window and switch light/dark themes; the text viewport remains aligned and readable.

---

### Task 4: Full Regression Verification

**Files:**
- Verify only; no planned production edits.

**Interfaces:**
- Consumes the finished row-routing helpers, Quick Preview helpers, and native text-control lifecycle.
- Produces fresh build and test evidence.

- [ ] **Step 1: Run formatting and placeholder checks**

Run:

```powershell
git diff --check
rg -n "T[B]D|T[O]DO|P[L]ACEHOLDER" docs/superpowers/plans/2026-07-15-finderx-row-selection-quick-preview.md
```

Expected: `git diff --check` exits 0 and the placeholder scan finds no matches.

- [ ] **Step 2: Run the complete build and CTest suite**

Run:

```powershell
.\scripts\build.ps1 -RunTests
```

Expected: CMake build exits 0 and CTest reports 100% tests passed with zero failed tests.

- [ ] **Step 3: Review only task-relevant diffs**

Run:

```powershell
git diff -- src/ui/FinderListView.h src/ui/FinderListView.cpp src/ui/FinderListViewTests.cpp src/ui/QuickPreview.h src/ui/QuickPreview.cpp src/ui/QuickPreviewTests.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp CMakeLists.txt
git status --short
```

Expected: the requested changes are present, unrelated user changes remain intact, and no generated build output is tracked.

- [ ] **Step 4: Report evidence and manual-test limits**

Report the focused test outputs, full CTest count, application build result, files changed, and whether interactive manual verification was performed. Do not claim unperformed manual checks as complete.
