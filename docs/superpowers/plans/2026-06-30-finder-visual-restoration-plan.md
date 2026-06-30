# FinderX Finder Visual Restoration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make FinderX visually closer to the provided macOS Finder list-view screenshot without changing navigation or file-management behavior.

**Architecture:** Keep changes inside existing Direct2D UI layers. Add only minimal render helpers, then polish `FinderListView` row drawing and `FinderChrome` chrome drawing while preserving existing hit testing and state flow.

**Tech Stack:** C++20 conservative subset, CMake, MSVC, Win32, Direct2D/DirectWrite.

---

## File Structure

Modify:

- `src/ui/RenderContext.h`: add a minimal outline helper for rounded rectangles.
- `src/ui/RenderContext.cpp`: implement the outline helper using the existing brush.
- `src/ui/FinderListView.cpp`: draw folder/file icons, tune row spacing and list colors, remove `[D]`/`[F]` markers.
- `src/ui/FinderChrome.cpp`: draw sidebar icons, polish toolbar controls, and render segmented path bar.

No model, filesystem, navigation, or CMake changes are required unless verification exposes a compile issue.

---

### Task 1: Minimal Render Outline Helper

**Files:**
- Modify: `src/ui/RenderContext.h`
- Modify: `src/ui/RenderContext.cpp`

- [ ] **Step 1: Add rounded outline API**

Modify `src/ui/RenderContext.h` public methods:

```cpp
    void drawRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color, float width = 1.0f);
```

- [ ] **Step 2: Implement rounded outline**

Add to `src/ui/RenderContext.cpp` after `fillRoundedRect`:

```cpp
void RenderContext::drawRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color, float width) {
    if (!target_ || !brush_ || width <= 0.0f || rect.rect.left >= rect.rect.right || rect.rect.top >= rect.rect.bottom) {
        return;
    }

    brush_->SetColor(color);
    target_->DrawRoundedRectangle(rect, brush_.Get(), width);
}
```

- [ ] **Step 3: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: `D:\finderX\build\Debug\FinderX.exe` builds.

- [ ] **Step 4: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/RenderContext.h src/ui/RenderContext.cpp
git -c safe.directory=D:/finderX commit -m "feat: add rounded outline rendering"
```

---

### Task 2: Finder-Style List Row Icons

**Files:**
- Modify: `src/ui/FinderListView.cpp`

- [ ] **Step 1: Add icon layout constants**

In the anonymous namespace of `src/ui/FinderListView.cpp`, add:

```cpp
constexpr float kIndentPerDepth = 18.0f;
constexpr float kNameStartOffset = 32.0f;
constexpr float kIconSize = 14.0f;
constexpr float kIconTextGap = 5.0f;
```

Replace repeated `18.0f` depth indentation usage with `kIndentPerDepth` where it is part of row indentation.

- [ ] **Step 2: Add list icon helpers**

Add these helpers in the anonymous namespace:

```cpp
void drawDisclosure(RenderContext& render, const FileNode& node, float x, float y, D2D1_COLOR_F color) {
    if (node.kind != FileKind::Folder) {
        return;
    }

    const wchar_t* arrow = node.expanded ? L"\u25BE" : L"\u25B8";
    render.drawText(arrow, D2D1::RectF(x, y, x + 14.0f, y + 16.0f), render.textFormat(), color);
}

void drawFolderIcon(RenderContext& render, float x, float y, bool selected) {
    const D2D1_COLOR_F tab = selected ? D2D1::ColorF(0.86f, 0.95f, 1.0f) : D2D1::ColorF(0.45f, 0.76f, 0.93f);
    const D2D1_COLOR_F body = selected ? D2D1::ColorF(0.72f, 0.88f, 1.0f) : D2D1::ColorF(0.36f, 0.70f, 0.90f);
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 1.0f, y + 3.0f, x + 8.0f, y + 6.5f), 1.0f, 1.0f), tab);
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x, y + 5.0f, x + 15.0f, y + 14.0f), 1.8f, 1.8f), body);
}

void drawFileIcon(RenderContext& render, float x, float y, bool selected) {
    const D2D1_COLOR_F fill = selected ? D2D1::ColorF(0.96f, 0.98f, 1.0f) : D2D1::ColorF(0.98f, 0.98f, 0.98f);
    const D2D1_COLOR_F outline = selected ? D2D1::ColorF(0.78f, 0.88f, 1.0f) : D2D1::ColorF(0.70f, 0.70f, 0.70f);
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 2.0f, y + 1.0f, x + 13.0f, y + 15.0f), 1.2f, 1.2f), fill);
    render.drawRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 2.0f, y + 1.0f, x + 13.0f, y + 15.0f), 1.2f, 1.2f), outline);
    render.drawLine(D2D1::Point2F(x + 5.0f, y + 6.0f), D2D1::Point2F(x + 10.5f, y + 6.0f), outline);
    render.drawLine(D2D1::Point2F(x + 5.0f, y + 9.0f), D2D1::Point2F(x + 10.5f, y + 9.0f), outline);
}

void drawNodeIcon(RenderContext& render, const FileNode& node, float x, float y, bool selected) {
    if (node.kind == FileKind::Folder) {
        drawFolderIcon(render, x, y, selected);
    } else {
        drawFileIcon(render, x, y, selected);
    }
}
```

- [ ] **Step 3: Replace text markers in row drawing**

In `FinderListView::draw`, replace:

```cpp
        const float nameX = bounds.left + 32.0f + static_cast<float>(visible.depth) * 18.0f;
        const wchar_t* arrow = node.kind == FileKind::Folder ? (node.expanded ? L"\u25BE" : L"\u25B8") : L" ";
        const wchar_t* marker = node.kind == FileKind::Folder ? L"[D]" : L"[F]";
        const std::wstring name = std::wstring(arrow) + L" " + marker + L" " + node.name;

        drawTextIfWide(render, name, D2D1::RectF(nameX, y + 2.0f, nameRight, y + kRowHeight + 2.0f), render.textFormat(), textColor);
```

with:

```cpp
        const float rowIndent = static_cast<float>(visible.depth) * kIndentPerDepth;
        const float disclosureX = bounds.left + kNameStartOffset + rowIndent;
        const float iconX = disclosureX + kDisclosureWidth;
        const float iconY = y + 3.0f;
        const float textX = iconX + kIconSize + kIconTextGap;

        drawDisclosure(render, node, disclosureX, y + 2.0f, mutedColor);
        drawNodeIcon(render, node, iconX, iconY, selected);
        drawTextIfWide(render, node.name, D2D1::RectF(textX, y + 2.0f, nameRight, y + kRowHeight + 2.0f), render.textFormat(), textColor);
```

- [ ] **Step 4: Keep disclosure hit testing aligned**

Modify `hitTestDisclosure`:

```cpp
    const float disclosureLeft = bounds.left + kNameStartOffset + static_cast<float>(row.depth) * kIndentPerDepth;
```

Keep the width as `kDisclosureWidth`.

- [ ] **Step 5: Tune list colors lightly**

In `FinderListView::draw`, keep selected blue but use:

```cpp
D2D1::ColorF(0.00f, 0.42f, 0.88f)
```

for selected row fill, and use:

```cpp
D2D1::ColorF(0.965f, 0.965f, 0.965f)
```

for alternating rows.

- [ ] **Step 6: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: Debug app builds.

- [ ] **Step 7: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderListView.cpp
git -c safe.directory=D:/finderX commit -m "feat: draw finder-style list icons"
```

---

### Task 3: Finder-Style Chrome And Path Bar

**Files:**
- Modify: `src/ui/FinderChrome.cpp`

- [ ] **Step 1: Add sidebar icon helpers**

In the anonymous namespace of `src/ui/FinderChrome.cpp`, add:

```cpp
void drawSidebarIcon(RenderContext& render, std::wstring_view label, float x, float y, bool selected, bool available) {
    const D2D1_COLOR_F color = available
        ? (selected ? D2D1::ColorF(0.08f, 0.38f, 0.78f) : D2D1::ColorF(0.05f, 0.48f, 0.86f))
        : D2D1::ColorF(0.58f, 0.58f, 0.58f);
    const D2D1_COLOR_F accent = available ? D2D1::ColorF(0.98f, 0.72f, 0.08f) : D2D1::ColorF(0.66f, 0.66f, 0.66f);

    if (label == L"Downloads") {
        render.drawLine(D2D1::Point2F(x + 7.0f, y + 2.0f), D2D1::Point2F(x + 7.0f, y + 11.0f), color, 1.8f);
        render.drawLine(D2D1::Point2F(x + 3.5f, y + 7.5f), D2D1::Point2F(x + 7.0f, y + 11.0f), color, 1.8f);
        render.drawLine(D2D1::Point2F(x + 10.5f, y + 7.5f), D2D1::Point2F(x + 7.0f, y + 11.0f), color, 1.8f);
        render.drawLine(D2D1::Point2F(x + 3.0f, y + 13.0f), D2D1::Point2F(x + 11.0f, y + 13.0f), color, 1.4f);
        return;
    }

    if (label == L"Home") {
        render.drawLine(D2D1::Point2F(x + 2.0f, y + 8.0f), D2D1::Point2F(x + 7.0f, y + 3.0f), color, 1.6f);
        render.drawLine(D2D1::Point2F(x + 7.0f, y + 3.0f), D2D1::Point2F(x + 12.0f, y + 8.0f), color, 1.6f);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 3.0f, y + 8.0f, x + 11.0f, y + 14.0f), 1.2f, 1.2f), color);
        return;
    }

    if (label == L"Applications") {
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 2.0f, y + 2.0f, x + 6.0f, y + 6.0f), 1.0f, 1.0f), color);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 8.0f, y + 2.0f, x + 12.0f, y + 6.0f), 1.0f, 1.0f), color);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 2.0f, y + 8.0f, x + 6.0f, y + 12.0f), 1.0f, 1.0f), color);
        render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 8.0f, y + 8.0f, x + 12.0f, y + 12.0f), 1.0f, 1.0f), color);
        return;
    }

    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 1.0f, y + 4.0f, x + 13.0f, y + 14.0f), 1.8f, 1.8f), color);
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(x + 2.0f, y + 2.0f, x + 8.0f, y + 5.0f), 1.0f, 1.0f), accent);
}
```

- [ ] **Step 2: Draw sidebar icons and adjust text**

In the sidebar item loop, before label text, add:

```cpp
        drawSidebarIcon(render, item.label, 35.0f, rowY + 4.0f, item.selected, item.available);
```

Change the label rect from:

```cpp
D2D1::RectF(kSidebarTextLeft, rowY + 2.0f, 160.0f, rowY + 22.0f)
```

to:

```cpp
D2D1::RectF(56.0f, rowY + 2.0f, 160.0f, rowY + 22.0f)
```

- [ ] **Step 3: Add path splitting helper**

Add:

```cpp
std::vector<std::wstring> splitPathSegments(std::wstring_view path) {
    std::vector<std::wstring> segments;
    std::wstring current;
    for (wchar_t ch : path) {
        if (ch == L'\\' || ch == L'/') {
            if (!current.empty()) {
                segments.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        segments.push_back(current);
    }
    return segments;
}
```

Add `#include <string>` and `#include <vector>` if missing.

- [ ] **Step 4: Add path segment drawing helper**

Add:

```cpp
void drawPathSegments(RenderContext& render, const D2D1_RECT_F& rect, std::wstring_view path) {
    const std::vector<std::wstring> segments = splitPathSegments(path);
    float x = rect.left + 14.0f;
    const float y = rect.top + 5.0f;
    for (std::size_t index = 0; index < segments.size() && x < rect.right - 20.0f; ++index) {
        const std::wstring& segment = segments[index];
        const float segmentWidth = (std::min)(120.0f, 8.0f + static_cast<float>(segment.size()) * 7.0f);
        render.drawText(segment, D2D1::RectF(x, y, (std::min)(x + segmentWidth, rect.right - 12.0f), rect.bottom), render.textFormat(), D2D1::ColorF(0.34f, 0.34f, 0.34f));
        x += segmentWidth;
        if (index + 1 < segments.size() && x < rect.right - 20.0f) {
            render.drawText(L"\u203a", D2D1::RectF(x, y, x + 16.0f, rect.bottom), render.textFormat(), D2D1::ColorF(0.48f, 0.48f, 0.48f));
            x += 16.0f;
        }
    }
}
```

- [ ] **Step 5: Replace pathbar text drawing**

Replace the final `drawTextClipped` pathbar block with:

```cpp
    const D2D1_RECT_F pathTextRect = D2D1::RectF(
        rects.pathbar.left,
        rects.pathbar.top,
        rects.pathbar.right,
        rects.pathbar.bottom);
    if (state.statusText.empty()) {
        drawPathSegments(render, pathTextRect, state.pathText);
    } else {
        drawTextClipped(
            render,
            state.statusText,
            D2D1::RectF(rects.pathbar.left + 14.0f, rects.pathbar.top + 5.0f, rects.pathbar.right - 12.0f, rects.pathbar.bottom),
            window,
            render.textFormat(),
            D2D1::ColorF(0.72f, 0.18f, 0.16f));
    }
```

- [ ] **Step 6: Polish toolbar back/forward clickable visual**

Before drawing each chevron, draw a subtle hoverless button background:

```cpp
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(rects.toolbar.left + 16.0f, 14.0f, rects.toolbar.left + 48.0f, 42.0f), 6.0f, 6.0f),
        D2D1::ColorF(0.93f, 0.93f, 0.92f));
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(rects.toolbar.left + 52.0f, 14.0f, rects.toolbar.left + 84.0f, 42.0f), 6.0f, 6.0f),
        D2D1::ColorF(0.93f, 0.93f, 0.92f));
```

Keep the existing chevron hit testing unchanged.

- [ ] **Step 7: Build app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
```

Expected: Debug app builds.

- [ ] **Step 8: Commit**

Run:

```powershell
git -c safe.directory=D:/finderX add src/ui/FinderChrome.cpp
git -c safe.directory=D:/finderX commit -m "feat: polish finder chrome visuals"
```

---

### Task 4: Verification And Visual Smoke Check

**Files:**
- Modify only if verification exposes a defect.

- [ ] **Step 1: Configure**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build -G 'Visual Studio 18 2026' -A x64
```

Expected: configure and generate succeed.

- [ ] **Step 2: Build Debug app and tests**

Run sequentially:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target FinderX
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_model_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_fs_tests
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Debug --target finderx_navigation_tests
```

Expected: all targets build.

- [ ] **Step 3: Run tests**

Run:

```powershell
.\build\Debug\finderx_model_tests.exe
.\build\Debug\finderx_fs_tests.exe
.\build\Debug\finderx_navigation_tests.exe
```

Expected:

```text
FileTreeTests passed
DirectoryLoaderTests passed
NavigationHistoryTests passed
```

- [ ] **Step 4: Build Release app**

Run:

```powershell
& 'D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target FinderX
```

Expected:

- `D:\finderX\build\Debug\FinderX.exe`
- `D:\finderX\build\Release\FinderX.exe`

- [ ] **Step 5: Short app launch**

Run:

```powershell
$process = Start-Process -FilePath .\build\Debug\FinderX.exe -PassThru
Start-Sleep -Seconds 3
$alive = -not $process.HasExited
if ($alive) {
    Stop-Process -Id $process.Id -Force
    'FinderX started and remained running for 3 seconds'
} else {
    "FinderX exited early with code $($process.ExitCode)"
    exit 1
}
```

Expected:

```text
FinderX started and remained running for 3 seconds
```

- [ ] **Step 6: Manual visual checklist**

Open `D:\finderX\build\Debug\FinderX.exe` and check:

- List rows show drawn folder/file icons.
- No `[D]` or `[F]` markers appear.
- Sidebar rows show small icons.
- Path bar displays separated path segments when no status is active.
- Back/Forward buttons still respond when enabled.
- Sidebar navigation still responds.
- Disclosure expand/collapse still responds.
- Double-click and Enter activation still work.
- At default 1188x768 size, text does not visibly overlap.

- [ ] **Step 7: Static checks**

Run:

```powershell
git -c safe.directory=D:/finderX diff --check
git -c safe.directory=D:/finderX status --short
```

Expected: no whitespace errors; only pre-existing intentional untracked files may remain.

- [ ] **Step 8: Commit verification fixes if needed**

If fixes were needed:

```powershell
git -c safe.directory=D:/finderX add src/ui
git -c safe.directory=D:/finderX commit -m "fix: stabilize finder visual restoration"
```

If no fixes were needed, do not create an empty commit.
