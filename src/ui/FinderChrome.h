#pragma once

#include "navigation/SidebarModel.h"
#include "ui/RenderContext.h"

#include <cstddef>
#include <string>

#include <d2d1.h>

namespace finderx {

struct LayoutRects {
    D2D1_RECT_F sidebar{};
    D2D1_RECT_F toolbar{};
    D2D1_RECT_F header{};
    D2D1_RECT_F list{};
    D2D1_RECT_F pathbar{};
};

enum class ChromeHitKind {
    None,
    Back,
    Forward,
    SearchField,
    SidebarItem,
    Tab,
    CloseTab,
    NewTab,
    NewFolder,
    NewFile,
    SortMenu,
    Settings,
    HeaderName,
    HeaderModified,
    HeaderSize,
    HeaderKind,
    ResizeModifiedColumn,
    ResizeSizeColumn,
    ResizeKindColumn,
    PathSegment,
    AddressField
};

struct ChromeHitResult {
    ChromeHitKind kind = ChromeHitKind::None;
    std::size_t sidebarIndex = 0;
    std::size_t tabIndex = 0;
    std::wstring path;
};

class FinderChrome {
public:
    LayoutRects layout(float width, float height) const;
    void draw(RenderContext& render, const LayoutRects& rects);
    void draw(RenderContext& render, const LayoutRects& rects, const ChromeState& state);
    ChromeHitResult hitTest(float x, float y, const LayoutRects& rects, const ChromeState& state) const;
};

}
