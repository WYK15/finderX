# FinderX C++ MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first native FinderX milestone: a Windows C++ app with a Finder-like window, sidebar, toolbar, path bar, and an interactive hierarchical list using sample data.

**Architecture:** The MVP starts with a pure model layer that can be tested without Win32, then adds a Win32/Direct2D shell and a custom-rendered Finder-style list. Real filesystem loading is intentionally behind interfaces so later tasks can replace sample data without rewriting rendering or interaction code.

**Tech Stack:** C++20 conservative subset, CMake, MSVC, Win32 API, Direct2D, DirectWrite, Windows SDK.

---

## File Structure

Create these files:

- `CMakeLists.txt`: root build configuration.
- `.gitignore`: excludes build outputs and local preview artifacts.
- `src/main.cpp`: WinMain entry point and application startup.
- `src/app/App.h`, `src/app/App.cpp`: app lifetime, COM initialization, window startup.
- `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp`: Win32 window class, message dispatch, layout orchestration.
- `src/ui/RenderContext.h`, `src/ui/RenderContext.cpp`: Direct2D/DirectWrite resources and drawing helpers.
- `src/ui/FinderChrome.h`, `src/ui/FinderChrome.cpp`: sidebar, toolbar, table header, path bar drawing.
- `src/ui/FinderListView.h`, `src/ui/FinderListView.cpp`: hierarchical list drawing, selection, scrolling, expand/collapse hit testing.
- `src/model/FileNode.h`: file node types and metadata.
- `src/model/FileTree.h`, `src/model/FileTree.cpp`: tree storage, sample data, flattening expanded nodes.
- `src/model/FileTreeTests.cpp`: small console test executable for model behavior.

The first milestone does not create a filesystem loader. That comes after the list UI and model are stable.

---

### Task 1: Toolchain And Repository Setup

**Files:**
- Create: `.gitignore`
- Create: `CMakeLists.txt`
- Create: `src/model/FileNode.h`
- Create: `src/model/FileTree.h`
- Create: `src/model/FileTree.cpp`
- Create: `src/model/FileTreeTests.cpp`

- [ ] **Step 1: Verify required tools**

Run in a terminal that has Visual Studio Build Tools loaded:

```powershell
cmake --version
cl
```

Expected:

- `cmake --version` prints a version.
- `cl` prints Microsoft compiler information.

If `cl` is not found, open "x64 Native Tools Command Prompt for VS 2022" or run the equivalent Visual Studio Developer PowerShell.

- [ ] **Step 2: Initialize git if needed**

Run:

```powershell
git status
```

If the output says `fatal: not a git repository`, run:

```powershell
git init
```

Expected: git initializes a repository in `D:\finderX`.

- [ ] **Step 3: Add `.gitignore`**

Create `.gitignore`:

```gitignore
build/
out/
.vs/
*.user
*.suo
*.VC.db
*.VC.VC.opendb
.superpowers/
```

- [ ] **Step 4: Add root `CMakeLists.txt`**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

project(FinderX LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(finderx_model
    src/model/FileTree.cpp
)

target_include_directories(finderx_model PUBLIC src)

add_executable(finderx_model_tests
    src/model/FileTreeTests.cpp
)

target_link_libraries(finderx_model_tests PRIVATE finderx_model)

add_executable(FinderX WIN32
    src/main.cpp
    src/app/App.cpp
    src/ui/MainWindow.cpp
    src/ui/RenderContext.cpp
    src/ui/FinderChrome.cpp
    src/ui/FinderListView.cpp
)

target_include_directories(FinderX PRIVATE src)
target_link_libraries(FinderX PRIVATE
    finderx_model
    d2d1
    dwrite
    windowscodecs
    ole32
    shell32
)
```

- [ ] **Step 5: Add model type header**

Create `src/model/FileNode.h`:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace finderx {

using NodeId = std::uint32_t;
inline constexpr NodeId kInvalidNodeId = 0;

enum class FileKind {
    Folder,
    File,
};

struct FileNode {
    NodeId id = kInvalidNodeId;
    NodeId parent = kInvalidNodeId;
    std::wstring name;
    std::wstring modified;
    std::wstring size;
    std::wstring kindText;
    FileKind kind = FileKind::File;
    bool expanded = false;
    std::vector<NodeId> children;
};

struct VisibleRow {
    NodeId nodeId = kInvalidNodeId;
    int depth = 0;
};

}
```

- [ ] **Step 6: Add file tree interface**

Create `src/model/FileTree.h`:

```cpp
#pragma once

#include "model/FileNode.h"

#include <span>
#include <vector>

namespace finderx {

class FileTree {
public:
    FileTree();

    NodeId rootId() const;
    const FileNode& node(NodeId id) const;
    FileNode& node(NodeId id);
    std::span<const FileNode> nodes() const;

    NodeId addNode(NodeId parent, std::wstring name, FileKind kind,
                   std::wstring modified, std::wstring size, std::wstring kindText);
    void setExpanded(NodeId id, bool expanded);
    void toggleExpanded(NodeId id);
    std::vector<VisibleRow> flatten() const;

    static FileTree sample();

private:
    void flattenFrom(NodeId id, int depth, std::vector<VisibleRow>& rows) const;

    std::vector<FileNode> nodes_;
    NodeId rootId_ = kInvalidNodeId;
};

}
```

- [ ] **Step 7: Add file tree implementation**

Create `src/model/FileTree.cpp`:

```cpp
#include "model/FileTree.h"

#include <stdexcept>

namespace finderx {

FileTree::FileTree() {
    nodes_.push_back(FileNode{});
    rootId_ = addNode(kInvalidNodeId, L"home", FileKind::Folder, L"", L"--", L"文件夹");
    nodes_[rootId_].expanded = true;
}

NodeId FileTree::rootId() const {
    return rootId_;
}

const FileNode& FileTree::node(NodeId id) const {
    if (id == kInvalidNodeId || id >= nodes_.size()) {
        throw std::out_of_range("invalid node id");
    }
    return nodes_[id];
}

FileNode& FileTree::node(NodeId id) {
    if (id == kInvalidNodeId || id >= nodes_.size()) {
        throw std::out_of_range("invalid node id");
    }
    return nodes_[id];
}

std::span<const FileNode> FileTree::nodes() const {
    return nodes_;
}

NodeId FileTree::addNode(NodeId parent, std::wstring name, FileKind kind,
                         std::wstring modified, std::wstring size, std::wstring kindText) {
    const NodeId id = static_cast<NodeId>(nodes_.size());
    FileNode node;
    node.id = id;
    node.parent = parent;
    node.name = std::move(name);
    node.kind = kind;
    node.modified = std::move(modified);
    node.size = std::move(size);
    node.kindText = std::move(kindText);
    nodes_.push_back(std::move(node));
    if (parent != kInvalidNodeId) {
        this->node(parent).children.push_back(id);
    }
    return id;
}

void FileTree::setExpanded(NodeId id, bool expanded) {
    FileNode& item = node(id);
    if (item.kind == FileKind::Folder) {
        item.expanded = expanded;
    }
}

void FileTree::toggleExpanded(NodeId id) {
    FileNode& item = node(id);
    if (item.kind == FileKind::Folder) {
        item.expanded = !item.expanded;
    }
}

std::vector<VisibleRow> FileTree::flatten() const {
    std::vector<VisibleRow> rows;
    for (NodeId child : node(rootId_).children) {
        flattenFrom(child, 0, rows);
    }
    return rows;
}

void FileTree::flattenFrom(NodeId id, int depth, std::vector<VisibleRow>& rows) const {
    const FileNode& item = node(id);
    rows.push_back(VisibleRow{id, depth});
    if (item.kind != FileKind::Folder || !item.expanded) {
        return;
    }
    for (NodeId child : item.children) {
        flattenFrom(child, depth + 1, rows);
    }
}

FileTree FileTree::sample() {
    FileTree tree;
    NodeId environments = tree.addNode(tree.rootId(), L"environments", FileKind::Folder, L"2025年7月28日 22:32", L"--", L"文件夹");
    NodeId androidEnv = tree.addNode(environments, L"androidEnv", FileKind::Folder, L"2025年7月28日 22:32", L"--", L"文件夹");
    NodeId platformTools = tree.addNode(androidEnv, L"platform-tools", FileKind::Folder, L"2025年7月28日 22:32", L"--", L"文件夹");
    tree.addNode(platformTools, L"adb", FileKind::File, L"2025年4月24日 02:33", L"15.6 MB", L"Unix 可执行文件");
    tree.addNode(platformTools, L"etc1tool", FileKind::File, L"2025年4月24日 02:33", L"688 KB", L"Unix 可执行文件");
    tree.addNode(platformTools, L"fastboot", FileKind::File, L"2025年4月24日 02:33", L"4.8 MB", L"Unix 可执行文件");
    tree.addNode(platformTools, L"hprof-conv", FileKind::File, L"2025年4月24日 02:33", L"135 KB", L"Unix 可执行文件");
    tree.addNode(androidEnv, L"lib64", FileKind::Folder, L"2025年4月24日 02:33", L"--", L"文件夹");
    tree.addNode(platformTools, L"NOTICE.txt", FileKind::File, L"2008年1月1日 15:00", L"1.1 MB", L"纯文本文稿");
    tree.addNode(tree.rootId(), L"debugserver-16.5", FileKind::Folder, L"2025年7月26日 23:54", L"--", L"文件夹");
    tree.addNode(tree.rootId(), L"MonkeyDev", FileKind::Folder, L"2024年6月18日 21:50", L"--", L"文件夹");
    NodeId projects = tree.addNode(tree.rootId(), L"projects", FileKind::Folder, L"2026年6月4日 23:01", L"--", L"文件夹");
    tree.addNode(projects, L"4utool", FileKind::Folder, L"2026年5月30日 00:02", L"--", L"文件夹");
    tree.addNode(projects, L"codexopen", FileKind::Folder, L"2026年5月29日 23:57", L"--", L"文件夹");
    tree.addNode(projects, L"readme.md", FileKind::File, L"2026年5月29日 23:57", L"551 字节", L"Markdown Text");
    tree.setExpanded(environments, true);
    tree.setExpanded(androidEnv, true);
    tree.setExpanded(platformTools, true);
    tree.setExpanded(projects, true);
    return tree;
}

}
```

- [ ] **Step 8: Add model tests**

Create `src/model/FileTreeTests.cpp`:

```cpp
#include "model/FileTree.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

int main() {
    FileTree tree = FileTree::sample();
    auto rows = tree.flatten();
    require(!rows.empty(), "sample tree should flatten to visible rows");
    require(tree.node(rows.front().nodeId).name == L"environments", "first visible row should be environments");

    const std::size_t expandedCount = rows.size();
    tree.toggleExpanded(rows.front().nodeId);
    rows = tree.flatten();
    require(rows.size() < expandedCount, "collapsing environments should reduce visible row count");

    tree.toggleExpanded(rows.front().nodeId);
    rows = tree.flatten();
    require(rows.size() == expandedCount, "expanding environments should restore visible row count");

    std::cout << "FileTreeTests passed\n";
    return 0;
}
```

- [ ] **Step 9: Configure and build tests**

Run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target finderx_model_tests
```

Expected: build succeeds and creates `build\Debug\finderx_model_tests.exe`.

- [ ] **Step 10: Run model tests**

Run:

```powershell
.\build\Debug\finderx_model_tests.exe
```

Expected:

```text
FileTreeTests passed
```

- [ ] **Step 11: Commit**

Run:

```powershell
git add .gitignore CMakeLists.txt src/model
git commit -m "chore: add FinderX model foundation"
```

---

### Task 2: Win32 App Skeleton

**Files:**
- Create: `src/main.cpp`
- Create: `src/app/App.h`
- Create: `src/app/App.cpp`
- Create: `src/ui/MainWindow.h`
- Create: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add app interface**

Create `src/app/App.h`:

```cpp
#pragma once

#include <windows.h>

namespace finderx {

class App {
public:
    int run(HINSTANCE instance, int showCommand);
};

}
```

- [ ] **Step 2: Add main window interface**

Create `src/ui/MainWindow.h`:

```cpp
#pragma once

#include <windows.h>

namespace finderx {

class MainWindow {
public:
    bool create(HINSTANCE instance, int showCommand);
    HWND hwnd() const;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
};

}
```

- [ ] **Step 3: Add main window implementation**

Create `src/ui/MainWindow.cpp`:

```cpp
#include "ui/MainWindow.h"

namespace finderx {

namespace {
constexpr wchar_t kWindowClassName[] = L"FinderXMainWindow";
}

bool MainWindow::create(HINSTANCE instance, int showCommand) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kWindowClassName;

    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        0,
        kWindowClassName,
        L"FinderX",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1188,
        768,
        nullptr,
        nullptr,
        instance,
        this);

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, showCommand);
    UpdateWindow(hwnd_);
    return true;
}

HWND MainWindow::hwnd() const {
    return hwnd_;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MainWindow* window = nullptr;
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        window = static_cast<MainWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->hwnd_ = hwnd;
    } else {
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->handleMessage(message, wParam, lParam);
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);
        RECT rect;
        GetClientRect(hwnd_, &rect);
        FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        EndPaint(hwnd_, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd_, message, wParam, lParam);
    }
}

}
```

- [ ] **Step 4: Add app implementation**

Create `src/app/App.cpp`:

```cpp
#include "app/App.h"

#include "ui/MainWindow.h"

#include <objbase.h>

namespace finderx {

int App::run(HINSTANCE instance, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    MainWindow window;
    if (!window.create(instance, showCommand)) {
        CoUninitialize();
        return 1;
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return static_cast<int>(msg.wParam);
}

}
```

- [ ] **Step 5: Add WinMain**

Create `src/main.cpp`:

```cpp
#include "app/App.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    finderx::App app;
    return app.run(instance, showCommand);
}
```

- [ ] **Step 6: Build app**

Run:

```powershell
cmake --build build --config Debug --target FinderX
```

Expected: build succeeds and creates `build\Debug\FinderX.exe`.

- [ ] **Step 7: Run app**

Run:

```powershell
.\build\Debug\FinderX.exe
```

Expected: a blank native window titled `FinderX` opens and can be resized and closed.

- [ ] **Step 8: Commit**

Run:

```powershell
git add src/main.cpp src/app src/ui CMakeLists.txt
git commit -m "feat: add Win32 application shell"
```

---

### Task 3: Direct2D Render Context

**Files:**
- Create: `src/ui/RenderContext.h`
- Create: `src/ui/RenderContext.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add render context interface**

Create `src/ui/RenderContext.h`:

```cpp
#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <string_view>

namespace finderx {

class RenderContext {
public:
    bool initialize(HWND hwnd);
    void resize(UINT width, UINT height);
    void beginDraw();
    void endDraw();
    void clear(D2D1_COLOR_F color);

    ID2D1HwndRenderTarget* target() const;
    IDWriteTextFormat* textFormat() const;
    IDWriteTextFormat* headerTextFormat() const;

    void drawText(std::wstring_view text, const D2D1_RECT_F& rect, IDWriteTextFormat* format, D2D1_COLOR_F color);
    void fillRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color);
    void fillRect(const D2D1_RECT_F& rect, D2D1_COLOR_F color);
    void drawLine(D2D1_POINT_2F start, D2D1_POINT_2F end, D2D1_COLOR_F color, float width = 1.0f);

private:
    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> headerTextFormat_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;
};

}
```

- [ ] **Step 2: Add render context implementation**

Create `src/ui/RenderContext.cpp`:

```cpp
#include "ui/RenderContext.h"

#include <algorithm>

namespace finderx {

bool RenderContext::initialize(HWND hwnd) {
    RECT rect{};
    GetClientRect(hwnd, &rect);
    D2D1_SIZE_U size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf())))) {
        return false;
    }
    if (FAILED(d2dFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        target_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), brush_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(dwriteFactory_->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        13.0f,
        L"zh-CN",
        textFormat_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(dwriteFactory_->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"zh-CN",
        headerTextFormat_.GetAddressOf()))) {
        return false;
    }
    textFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    headerTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    return true;
}

void RenderContext::resize(UINT width, UINT height) {
    if (target_) {
        target_->Resize(D2D1::SizeU(width, height));
    }
}

void RenderContext::beginDraw() {
    target_->BeginDraw();
}

void RenderContext::endDraw() {
    target_->EndDraw();
}

void RenderContext::clear(D2D1_COLOR_F color) {
    target_->Clear(color);
}

ID2D1HwndRenderTarget* RenderContext::target() const {
    return target_.Get();
}

IDWriteTextFormat* RenderContext::textFormat() const {
    return textFormat_.Get();
}

IDWriteTextFormat* RenderContext::headerTextFormat() const {
    return headerTextFormat_.Get();
}

void RenderContext::drawText(std::wstring_view text, const D2D1_RECT_F& rect, IDWriteTextFormat* format, D2D1_COLOR_F color) {
    brush_->SetColor(color);
    target_->DrawTextW(text.data(), static_cast<UINT32>(text.size()), format, rect, brush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void RenderContext::fillRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color) {
    brush_->SetColor(color);
    target_->FillRoundedRectangle(rect, brush_.Get());
}

void RenderContext::fillRect(const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    brush_->SetColor(color);
    target_->FillRectangle(rect, brush_.Get());
}

void RenderContext::drawLine(D2D1_POINT_2F start, D2D1_POINT_2F end, D2D1_COLOR_F color, float width) {
    brush_->SetColor(color);
    target_->DrawLine(start, end, brush_.Get(), width);
}

}
```

- [ ] **Step 3: Add render context to main window**

Modify `src/ui/MainWindow.h`:

```cpp
#pragma once

#include "ui/RenderContext.h"

#include <windows.h>

namespace finderx {

class MainWindow {
public:
    bool create(HINSTANCE instance, int showCommand);
    HWND hwnd() const;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void paint();

    HWND hwnd_ = nullptr;
    RenderContext render_;
};

}
```

- [ ] **Step 4: Replace paint handling**

In `src/ui/MainWindow.cpp`, replace the `WM_PAINT` case with:

```cpp
    case WM_PAINT:
        paint();
        return 0;
    case WM_SIZE:
        render_.resize(LOWORD(lParam), HIWORD(lParam));
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
```

Add this method at the end of the namespace:

```cpp
void MainWindow::paint() {
    PAINTSTRUCT ps;
    BeginPaint(hwnd_, &ps);
    if (!render_.target()) {
        render_.initialize(hwnd_);
    }
    render_.beginDraw();
    render_.clear(D2D1::ColorF(0.98f, 0.98f, 0.98f));
    render_.drawText(L"FinderX", D2D1::RectF(24, 24, 240, 60), render_.textFormat(), D2D1::ColorF(0.1f, 0.1f, 0.1f));
    render_.endDraw();
    EndPaint(hwnd_, &ps);
}
```

- [ ] **Step 5: Build and run**

Run:

```powershell
cmake --build build --config Debug --target FinderX
.\build\Debug\FinderX.exe
```

Expected: window opens with `FinderX` rendered through DirectWrite.

- [ ] **Step 6: Commit**

Run:

```powershell
git add src/ui CMakeLists.txt
git commit -m "feat: add Direct2D render context"
```

---

### Task 4: Static Finder Chrome And List

**Files:**
- Create: `src/ui/FinderChrome.h`
- Create: `src/ui/FinderChrome.cpp`
- Create: `src/ui/FinderListView.h`
- Create: `src/ui/FinderListView.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Add chrome interface**

Create `src/ui/FinderChrome.h`:

```cpp
#pragma once

#include "ui/RenderContext.h"

#include <d2d1.h>

namespace finderx {

struct LayoutRects {
    D2D1_RECT_F sidebar{};
    D2D1_RECT_F toolbar{};
    D2D1_RECT_F header{};
    D2D1_RECT_F list{};
    D2D1_RECT_F pathbar{};
};

class FinderChrome {
public:
    LayoutRects layout(float width, float height) const;
    void draw(RenderContext& render, const LayoutRects& rects);
};

}
```

- [ ] **Step 2: Add chrome drawing**

Create `src/ui/FinderChrome.cpp`:

```cpp
#include "ui/FinderChrome.h"

namespace finderx {

LayoutRects FinderChrome::layout(float width, float height) const {
    LayoutRects r;
    r.sidebar = D2D1::RectF(0, 0, 174, height);
    r.toolbar = D2D1::RectF(174, 0, width, 53);
    r.header = D2D1::RectF(174, 53, width, 81);
    r.pathbar = D2D1::RectF(174, height - 28, width, height);
    r.list = D2D1::RectF(174, 81, width, height - 28);
    return r;
}

void FinderChrome::draw(RenderContext& render, const LayoutRects& rects) {
    render.fillRect(rects.sidebar, D2D1::ColorF(0.905f, 0.905f, 0.905f));
    render.fillRect(rects.toolbar, D2D1::ColorF(0.972f, 0.972f, 0.972f));
    render.fillRect(rects.header, D2D1::ColorF(1.0f, 1.0f, 1.0f));
    render.fillRect(rects.pathbar, D2D1::ColorF(0.985f, 0.985f, 0.985f));

    render.drawLine(D2D1::Point2F(174, 0), D2D1::Point2F(174, rects.sidebar.bottom), D2D1::ColorF(0.82f, 0.82f, 0.82f));
    render.drawLine(D2D1::Point2F(174, 53), D2D1::Point2F(rects.toolbar.right, 53), D2D1::ColorF(0.84f, 0.84f, 0.84f));
    render.drawLine(D2D1::Point2F(174, 81), D2D1::Point2F(rects.header.right, 81), D2D1::ColorF(0.88f, 0.88f, 0.88f));
    render.drawLine(D2D1::Point2F(174, rects.pathbar.top), D2D1::Point2F(rects.pathbar.right, rects.pathbar.top), D2D1::ColorF(0.88f, 0.88f, 0.88f));

    render.drawText(L"个人收藏", D2D1::RectF(18, 72, 150, 94), render.headerTextFormat(), D2D1::ColorF(0.58f, 0.58f, 0.58f));
    render.drawText(L"应用程序\n文稿\n桌面\nhome\n下载", D2D1::RectF(34, 96, 160, 230), render.textFormat(), D2D1::ColorF(0.18f, 0.18f, 0.18f));
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(28, 173, 164, 201), 6, 6), D2D1::ColorF(0.80f, 0.80f, 0.80f));
    render.drawText(L"home", D2D1::RectF(54, 178, 150, 198), render.textFormat(), D2D1::ColorF(0.12f, 0.12f, 0.12f));

    render.drawText(L"‹  ›", D2D1::RectF(196, 14, 260, 48), render.textFormat(), D2D1::ColorF(0.66f, 0.66f, 0.66f));
    render.drawText(L"home", D2D1::RectF(280, 17, 420, 48), render.headerTextFormat(), D2D1::ColorF(0.20f, 0.20f, 0.20f));
    render.drawText(L"▦  ☷  ▥  ▭     ⇄   ▦⌄   ⇧   •••⌄", D2D1::RectF(580, 17, 940, 48), render.textFormat(), D2D1::ColorF(0.36f, 0.36f, 0.36f));
    render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(rects.toolbar.right - 202, 17, rects.toolbar.right - 14, 45), 7, 7), D2D1::ColorF(1, 1, 1));
    render.drawText(L"⌕ 搜索", D2D1::RectF(rects.toolbar.right - 190, 22, rects.toolbar.right - 30, 44), render.textFormat(), D2D1::ColorF(0.55f, 0.55f, 0.55f));

    render.drawText(L"名称", D2D1::RectF(238, 59, 360, 78), render.headerTextFormat(), D2D1::ColorF(0.18f, 0.18f, 0.18f));
    render.drawText(L"修改日期", D2D1::RectF(rects.header.right - 390, 59, rects.header.right - 250, 78), render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
    render.drawText(L"大小", D2D1::RectF(rects.header.right - 230, 59, rects.header.right - 160, 78), render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
    render.drawText(L"种类", D2D1::RectF(rects.header.right - 130, 59, rects.header.right - 20, 78), render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));

    render.drawText(L"Macintosh HD › 用户 › leo › home › environments › androidEnv › platform-tools › adb",
        D2D1::RectF(188, rects.pathbar.top + 5, rects.pathbar.right - 12, rects.pathbar.bottom),
        render.textFormat(),
        D2D1::ColorF(0.36f, 0.36f, 0.36f));
}

}
```

- [ ] **Step 3: Add list view interface**

Create `src/ui/FinderListView.h`:

```cpp
#pragma once

#include "model/FileTree.h"
#include "ui/RenderContext.h"

#include <d2d1.h>

namespace finderx {

class FinderListView {
public:
    explicit FinderListView(FileTree* tree);

    void draw(RenderContext& render, const D2D1_RECT_F& bounds);

private:
    FileTree* tree_ = nullptr;
    NodeId selected_ = kInvalidNodeId;
};

}
```

- [ ] **Step 4: Add static list drawing**

Create `src/ui/FinderListView.cpp`:

```cpp
#include "ui/FinderListView.h"

#include <string>

namespace finderx {

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {}

void FinderListView::draw(RenderContext& render, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return;
    }
    const auto rows = tree_->flatten();
    if (selected_ == kInvalidNodeId && !rows.empty()) {
        selected_ = rows.size() > 3 ? rows[3].nodeId : rows.front().nodeId;
    }

    constexpr float rowHeight = 20.0f;
    float y = bounds.top + 4.0f;
    const float dateX = bounds.right - 390.0f;
    const float sizeX = bounds.right - 230.0f;
    const float kindX = bounds.right - 130.0f;

    for (std::size_t i = 0; i < rows.size() && y + rowHeight <= bounds.bottom; ++i) {
        const VisibleRow& visible = rows[i];
        const FileNode& node = tree_->node(visible.nodeId);
        const bool selected = node.id == selected_;
        if (selected) {
            render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(bounds.left + 10, y, bounds.right - 10, y + rowHeight), 5, 5),
                D2D1::ColorF(0.0f, 0.43f, 0.90f));
        } else if (i % 2 == 1) {
            render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(bounds.left + 10, y, bounds.right - 10, y + rowHeight), 5, 5),
                D2D1::ColorF(0.955f, 0.955f, 0.955f));
        }

        const auto textColor = selected ? D2D1::ColorF(1, 1, 1) : D2D1::ColorF(0.10f, 0.10f, 0.10f);
        const auto mutedColor = selected ? D2D1::ColorF(1, 1, 1) : D2D1::ColorF(0.42f, 0.42f, 0.42f);
        const float nameX = bounds.left + 32.0f + static_cast<float>(visible.depth) * 18.0f;
        const wchar_t* arrow = node.kind == FileKind::Folder ? (node.expanded ? L"⌄" : L"›") : L"";
        const wchar_t* icon = node.kind == FileKind::Folder ? L"▣" : L"▰";
        std::wstring name = std::wstring(arrow) + L" " + icon + L" " + node.name;

        render.drawText(name, D2D1::RectF(nameX, y + 2, dateX - 8, y + rowHeight + 2), render.textFormat(), textColor);
        render.drawText(node.modified, D2D1::RectF(dateX, y + 2, sizeX - 6, y + rowHeight + 2), render.textFormat(), mutedColor);
        render.drawText(node.size, D2D1::RectF(sizeX, y + 2, kindX - 6, y + rowHeight + 2), render.textFormat(), mutedColor);
        render.drawText(node.kindText, D2D1::RectF(kindX, y + 2, bounds.right - 12, y + rowHeight + 2), render.textFormat(), mutedColor);

        y += rowHeight;
    }
}

}
```

- [ ] **Step 5: Wire chrome and list into main window**

Modify `src/ui/MainWindow.h` to include members:

```cpp
#include "model/FileTree.h"
#include "ui/FinderChrome.h"
#include "ui/FinderListView.h"
#include "ui/RenderContext.h"
```

Replace the class members with:

```cpp
    HWND hwnd_ = nullptr;
    RenderContext render_;
    FinderChrome chrome_;
    FileTree tree_ = FileTree::sample();
    FinderListView listView_{&tree_};
```

Modify `MainWindow::paint()`:

```cpp
void MainWindow::paint() {
    PAINTSTRUCT ps;
    BeginPaint(hwnd_, &ps);
    if (!render_.target()) {
        render_.initialize(hwnd_);
    }
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    const float width = static_cast<float>(rc.right - rc.left);
    const float height = static_cast<float>(rc.bottom - rc.top);
    const LayoutRects rects = chrome_.layout(width, height);

    render_.beginDraw();
    render_.clear(D2D1::ColorF(1, 1, 1));
    chrome_.draw(render_, rects);
    listView_.draw(render_, rects.list);
    render_.endDraw();
    EndPaint(hwnd_, &ps);
}
```

- [ ] **Step 6: Build and run**

Run:

```powershell
cmake --build build --config Debug --target FinderX
.\build\Debug\FinderX.exe
```

Expected: the app resembles the approved Finder list preview with sidebar, toolbar, column header, rows, selected row, and path bar.

- [ ] **Step 7: Commit**

Run:

```powershell
git add src/ui src/model CMakeLists.txt
git commit -m "feat: render Finder-style static surface"
```

---

### Task 5: List Selection And Expand/Collapse

**Files:**
- Modify: `src/ui/FinderListView.h`
- Modify: `src/ui/FinderListView.cpp`
- Modify: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Extend list view interface**

Modify `src/ui/FinderListView.h`:

```cpp
#pragma once

#include "model/FileTree.h"
#include "ui/RenderContext.h"

#include <d2d1.h>

namespace finderx {

class FinderListView {
public:
    explicit FinderListView(FileTree* tree);

    void draw(RenderContext& render, const D2D1_RECT_F& bounds);
    bool onMouseDown(float x, float y, const D2D1_RECT_F& bounds);
    bool onWheel(int wheelDelta);
    void onKeyDown(WPARAM key);

private:
    int hitRow(float y, const D2D1_RECT_F& bounds) const;
    void rebuildRows();

    FileTree* tree_ = nullptr;
    NodeId selected_ = kInvalidNodeId;
    std::vector<VisibleRow> rows_;
    float scrollY_ = 0.0f;
};

}
```

- [ ] **Step 2: Update list view implementation**

Replace `src/ui/FinderListView.cpp` with:

```cpp
#include "ui/FinderListView.h"

#include <algorithm>
#include <string>

namespace finderx {

namespace {
constexpr float kRowHeight = 20.0f;
}

FinderListView::FinderListView(FileTree* tree) : tree_(tree) {
    rebuildRows();
}

void FinderListView::rebuildRows() {
    rows_ = tree_ ? tree_->flatten() : std::vector<VisibleRow>{};
    if (selected_ == kInvalidNodeId && !rows_.empty()) {
        selected_ = rows_.front().nodeId;
    }
}

int FinderListView::hitRow(float y, const D2D1_RECT_F& bounds) const {
    const float localY = y - bounds.top - 4.0f + scrollY_;
    if (localY < 0) {
        return -1;
    }
    const int index = static_cast<int>(localY / kRowHeight);
    if (index < 0 || index >= static_cast<int>(rows_.size())) {
        return -1;
    }
    return index;
}

void FinderListView::draw(RenderContext& render, const D2D1_RECT_F& bounds) {
    if (!tree_) {
        return;
    }
    rebuildRows();
    const float dateX = bounds.right - 390.0f;
    const float sizeX = bounds.right - 230.0f;
    const float kindX = bounds.right - 130.0f;
    float y = bounds.top + 4.0f - scrollY_;

    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (y + kRowHeight < bounds.top) {
            y += kRowHeight;
            continue;
        }
        if (y > bounds.bottom) {
            break;
        }

        const VisibleRow& visible = rows_[i];
        const FileNode& node = tree_->node(visible.nodeId);
        const bool selected = node.id == selected_;
        if (selected) {
            render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(bounds.left + 10, y, bounds.right - 10, y + kRowHeight), 5, 5),
                D2D1::ColorF(0.0f, 0.43f, 0.90f));
        } else if (i % 2 == 1) {
            render.fillRoundedRect(D2D1::RoundedRect(D2D1::RectF(bounds.left + 10, y, bounds.right - 10, y + kRowHeight), 5, 5),
                D2D1::ColorF(0.955f, 0.955f, 0.955f));
        }

        const auto textColor = selected ? D2D1::ColorF(1, 1, 1) : D2D1::ColorF(0.10f, 0.10f, 0.10f);
        const auto mutedColor = selected ? D2D1::ColorF(1, 1, 1) : D2D1::ColorF(0.42f, 0.42f, 0.42f);
        const float nameX = bounds.left + 32.0f + static_cast<float>(visible.depth) * 18.0f;
        const wchar_t* arrow = node.kind == FileKind::Folder ? (node.expanded ? L"⌄" : L"›") : L"";
        const wchar_t* icon = node.kind == FileKind::Folder ? L"▣" : L"▰";
        std::wstring name = std::wstring(arrow) + L" " + icon + L" " + node.name;

        render.drawText(name, D2D1::RectF(nameX, y + 2, dateX - 8, y + kRowHeight + 2), render.textFormat(), textColor);
        render.drawText(node.modified, D2D1::RectF(dateX, y + 2, sizeX - 6, y + kRowHeight + 2), render.textFormat(), mutedColor);
        render.drawText(node.size, D2D1::RectF(sizeX, y + 2, kindX - 6, y + kRowHeight + 2), render.textFormat(), mutedColor);
        render.drawText(node.kindText, D2D1::RectF(kindX, y + 2, bounds.right - 12, y + kRowHeight + 2), render.textFormat(), mutedColor);
        y += kRowHeight;
    }
}

bool FinderListView::onMouseDown(float x, float y, const D2D1_RECT_F& bounds) {
    if (x < bounds.left || x > bounds.right || y < bounds.top || y > bounds.bottom) {
        return false;
    }
    const int index = hitRow(y, bounds);
    if (index < 0) {
        return false;
    }
    const VisibleRow row = rows_[index];
    selected_ = row.nodeId;

    const float nameX = bounds.left + 32.0f + static_cast<float>(row.depth) * 18.0f;
    if (x >= nameX && x <= nameX + 18.0f && tree_->node(row.nodeId).kind == FileKind::Folder) {
        tree_->toggleExpanded(row.nodeId);
        rebuildRows();
    }
    return true;
}

bool FinderListView::onWheel(int wheelDelta) {
    if (rows_.empty()) {
        return false;
    }
    scrollY_ -= static_cast<float>(wheelDelta) / WHEEL_DELTA * 48.0f;
    const float maxScroll = std::max(0.0f, static_cast<float>(rows_.size()) * kRowHeight - 200.0f);
    scrollY_ = std::clamp(scrollY_, 0.0f, maxScroll);
    return true;
}

void FinderListView::onKeyDown(WPARAM key) {
    rebuildRows();
    if (rows_.empty()) {
        return;
    }
    auto it = std::find_if(rows_.begin(), rows_.end(), [this](const VisibleRow& row) { return row.nodeId == selected_; });
    std::size_t index = it == rows_.end() ? 0 : static_cast<std::size_t>(it - rows_.begin());
    if (key == VK_DOWN && index + 1 < rows_.size()) {
        selected_ = rows_[index + 1].nodeId;
    } else if (key == VK_UP && index > 0) {
        selected_ = rows_[index - 1].nodeId;
    } else if (key == VK_RIGHT) {
        tree_->setExpanded(selected_, true);
    } else if (key == VK_LEFT) {
        tree_->setExpanded(selected_, false);
    }
}

}
```

- [ ] **Step 3: Dispatch input from main window**

In `src/ui/MainWindow.cpp`, add to `handleMessage`:

```cpp
    case WM_LBUTTONDOWN: {
        RECT rc{};
        GetClientRect(hwnd_, &rc);
        const LayoutRects rects = chrome_.layout(static_cast<float>(rc.right - rc.left), static_cast<float>(rc.bottom - rc.top));
        if (listView_.onMouseDown(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)), rects.list)) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
        if (listView_.onWheel(GET_WHEEL_DELTA_WPARAM(wParam))) {
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return 0;
    case WM_KEYDOWN:
        listView_.onKeyDown(wParam);
        InvalidateRect(hwnd_, nullptr, FALSE);
        return 0;
```

Also add:

```cpp
#include <windowsx.h>
```

- [ ] **Step 4: Build and verify interactions**

Run:

```powershell
cmake --build build --config Debug --target FinderX
.\build\Debug\FinderX.exe
```

Expected:

- Clicking rows changes selection.
- Clicking a folder disclosure arrow expands/collapses it.
- Mouse wheel scrolls when enough rows are present.
- Up/down keys move selection.
- Left/right keys collapse/expand selected folders.

- [ ] **Step 5: Commit**

Run:

```powershell
git add src/ui
git commit -m "feat: add Finder list interactions"
```

---

### Task 6: Milestone Verification

**Files:**
- Modify only if verification exposes a build or interaction defect.

- [ ] **Step 1: Run model tests**

Run:

```powershell
cmake --build build --config Debug --target finderx_model_tests
.\build\Debug\finderx_model_tests.exe
```

Expected:

```text
FileTreeTests passed
```

- [ ] **Step 2: Build app in Debug**

Run:

```powershell
cmake --build build --config Debug --target FinderX
```

Expected: Debug build succeeds.

- [ ] **Step 3: Build app in Release**

Run:

```powershell
cmake --build build --config Release --target FinderX
```

Expected: Release build succeeds.

- [ ] **Step 4: Manual visual check**

Run:

```powershell
.\build\Debug\FinderX.exe
```

Check:

- Window opens at roughly the same proportions as the approved browser preview.
- Sidebar, toolbar, table header, list, and path bar are visible.
- Rows are dense and Finder-like.
- Selected row spans all columns.
- Folder hierarchy is represented inside the main list, not in the sidebar.
- Resize does not visibly break the major regions.

- [ ] **Step 5: Commit verification fixes**

If any fixes were required, run:

```powershell
git add src CMakeLists.txt
git commit -m "fix: stabilize FinderX MVP milestone"
```

If no fixes were required, do not create an empty commit.

---

## Follow-Up Plan After This Milestone

After this milestone is working, write a second plan for real filesystem integration:

- Enumerate a real home directory.
- Replace sample data with filesystem nodes.
- Lazy-load children when expanding folders.
- Format Windows file times and sizes.
- Add Shell icons or a measured fallback icon system.
- Add search box text input and filtering.

