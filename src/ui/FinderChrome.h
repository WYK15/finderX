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
