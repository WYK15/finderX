#include "ui/FinderChrome.h"

#include <algorithm>

namespace finderx {

namespace {

constexpr float kSidebarWidth = 174.0f;
constexpr float kToolbarHeight = 53.0f;
constexpr float kHeaderBottom = 81.0f;
constexpr float kPathbarHeight = 28.0f;
constexpr float kMinTextWidth = 20.0f;

D2D1_COLOR_F rgb(float value) {
    return D2D1::ColorF(value, value, value);
}

float clampNonNegative(float value) {
    return (std::max)(0.0f, value);
}

bool hasArea(const D2D1_RECT_F& rect) {
    return rect.left < rect.right && rect.top < rect.bottom;
}

D2D1_RECT_F clampRect(const D2D1_RECT_F& rect, const D2D1_RECT_F& bounds) {
    return D2D1::RectF(
        (std::clamp)(rect.left, bounds.left, bounds.right),
        (std::clamp)(rect.top, bounds.top, bounds.bottom),
        (std::clamp)(rect.right, bounds.left, bounds.right),
        (std::clamp)(rect.bottom, bounds.top, bounds.bottom));
}

void drawTextClipped(
    RenderContext& render,
    std::wstring_view text,
    D2D1_RECT_F rect,
    const D2D1_RECT_F& bounds,
    IDWriteTextFormat* format,
    D2D1_COLOR_F color) {
    rect = clampRect(rect, bounds);
    if (rect.right - rect.left < kMinTextWidth || rect.top >= rect.bottom) {
        return;
    }

    render.drawText(text, rect, format, color);
}

void drawSeparator(RenderContext& render, float x, float top, float bottom) {
    if (top >= bottom) {
        return;
    }

    render.drawLine(
        D2D1::Point2F(x, top),
        D2D1::Point2F(x, bottom),
        D2D1::ColorF(0.82f, 0.82f, 0.82f));
}

}

LayoutRects FinderChrome::layout(float width, float height) const {
    const float right = clampNonNegative(width);
    const float bottom = clampNonNegative(height);
    const float contentLeft = (std::min)(kSidebarWidth, right);
    const float toolbarBottom = (std::min)(kToolbarHeight, bottom);
    const float headerBottom = (std::clamp)(kHeaderBottom, toolbarBottom, bottom);
    const float pathbarTop = (std::clamp)(bottom - kPathbarHeight, headerBottom, bottom);

    LayoutRects rects;
    rects.sidebar = D2D1::RectF(0.0f, 0.0f, contentLeft, bottom);
    rects.toolbar = D2D1::RectF(contentLeft, 0.0f, right, toolbarBottom);
    rects.header = D2D1::RectF(contentLeft, toolbarBottom, right, headerBottom);
    rects.pathbar = D2D1::RectF(contentLeft, pathbarTop, right, bottom);
    rects.list = D2D1::RectF(contentLeft, headerBottom, right, pathbarTop);
    return rects;
}

void FinderChrome::draw(RenderContext& render, const LayoutRects& rects) {
    const D2D1_RECT_F window = D2D1::RectF(0.0f, 0.0f, rects.pathbar.right, rects.sidebar.bottom);

    render.fillRect(rects.sidebar, D2D1::ColorF(0.92f, 0.93f, 0.94f));
    render.fillRect(rects.toolbar, D2D1::ColorF(0.97f, 0.97f, 0.965f));
    render.fillRect(rects.header, D2D1::ColorF(0.995f, 0.995f, 0.995f));
    render.fillRect(rects.pathbar, D2D1::ColorF(0.975f, 0.975f, 0.972f));

    if (rects.sidebar.right < rects.pathbar.right) {
        drawSeparator(render, rects.sidebar.right, 0.0f, rects.sidebar.bottom);
    }
    render.drawLine(
        D2D1::Point2F(rects.toolbar.left, rects.toolbar.bottom),
        D2D1::Point2F(rects.toolbar.right, rects.toolbar.bottom),
        rgb(0.82f));
    render.drawLine(
        D2D1::Point2F(rects.header.left, rects.header.bottom),
        D2D1::Point2F(rects.header.right, rects.header.bottom),
        rgb(0.86f));
    render.drawLine(
        D2D1::Point2F(rects.pathbar.left, rects.pathbar.top),
        D2D1::Point2F(rects.pathbar.right, rects.pathbar.top),
        rgb(0.86f));

    drawTextClipped(
        render,
        L"FAVORITES",
        D2D1::RectF(18.0f, 70.0f, 150.0f, 91.0f),
        rects.sidebar,
        render.headerTextFormat(),
        D2D1::ColorF(0.50f, 0.50f, 0.50f));
    drawTextClipped(
        render,
        L"Applications\nDocuments\nDesktop\nHome\nDownloads",
        D2D1::RectF(34.0f, 94.0f, 160.0f, 220.0f),
        rects.sidebar,
        render.textFormat(),
        D2D1::ColorF(0.16f, 0.16f, 0.16f));
    render.fillRoundedRect(
        D2D1::RoundedRect(clampRect(D2D1::RectF(24.0f, 174.0f, 164.0f, 200.0f), rects.sidebar), 6.0f, 6.0f),
        D2D1::ColorF(0.80f, 0.82f, 0.84f));
    drawTextClipped(
        render,
        L"Home",
        D2D1::RectF(54.0f, 178.0f, 150.0f, 198.0f),
        rects.sidebar,
        render.textFormat(),
        D2D1::ColorF(0.10f, 0.10f, 0.10f));

    drawTextClipped(
        render,
        L"\u2039   \u203a",
        D2D1::RectF(rects.toolbar.left + 22.0f, 15.0f, rects.toolbar.left + 92.0f, 43.0f),
        rects.toolbar,
        render.textFormat(),
        D2D1::ColorF(0.56f, 0.56f, 0.56f));
    drawTextClipped(
        render,
        L"home",
        D2D1::RectF(rects.toolbar.left + 106.0f, 16.0f, rects.toolbar.left + 240.0f, 45.0f),
        rects.toolbar,
        render.headerTextFormat(),
        D2D1::ColorF(0.18f, 0.18f, 0.18f));
    drawTextClipped(
        render,
        L"\u25A6   \u25A4   \u25A1   \u2022\u2022\u2022",
        D2D1::RectF(rects.toolbar.left + 380.0f, 17.0f, rects.toolbar.right - 232.0f, 44.0f),
        rects.toolbar,
        render.textFormat(),
        D2D1::ColorF(0.36f, 0.36f, 0.36f));

    if (rects.toolbar.right - rects.toolbar.left >= 240.0f) {
        const float searchLeft = (std::max)(rects.toolbar.left + 420.0f, rects.toolbar.right - 202.0f);
        const D2D1_RECT_F searchRect = clampRect(
            D2D1::RectF(searchLeft, 16.0f, rects.toolbar.right - 14.0f, 44.0f),
            rects.toolbar);
        if (searchRect.right - searchRect.left >= 80.0f) {
            render.fillRoundedRect(
                D2D1::RoundedRect(searchRect, 7.0f, 7.0f),
                D2D1::ColorF(1.0f, 1.0f, 1.0f));
            render.drawLine(
                D2D1::Point2F(searchRect.left, searchRect.top),
                D2D1::Point2F(searchRect.right, searchRect.top),
                rgb(0.88f));
            drawTextClipped(
                render,
                L"\u2315 Search",
                D2D1::RectF(searchRect.left + 11.0f, searchRect.top + 4.0f, searchRect.right - 12.0f, searchRect.bottom),
                searchRect,
                render.textFormat(),
                D2D1::ColorF(0.53f, 0.53f, 0.53f));
        }
    }

    const float headerWidth = rects.header.right - rects.header.left;
    if (hasArea(rects.header) && headerWidth >= 80.0f) {
        const float padding = 12.0f;
        const float dateWidth = headerWidth >= 520.0f ? 150.0f : 0.0f;
        const float sizeWidth = headerWidth >= 420.0f ? 80.0f : 0.0f;
        const float kindWidth = headerWidth >= 360.0f ? 120.0f : 0.0f;
        const float kindX = rects.header.right - padding - kindWidth;
        const float sizeX = kindX - sizeWidth;
        const float dateX = sizeX - dateWidth;
        const float trailingLeft = dateWidth > 0.0f
            ? dateX
            : (sizeWidth > 0.0f ? sizeX : (kindWidth > 0.0f ? kindX : rects.header.right - padding));
        const float nameRight = trailingLeft - 8.0f;

        drawTextClipped(render, L"Name", D2D1::RectF(rects.header.left + 64.0f, 59.0f, nameRight, 79.0f), rects.header, render.headerTextFormat(), D2D1::ColorF(0.18f, 0.18f, 0.18f));
        if (dateWidth > 0.0f) {
            drawTextClipped(render, L"Date Modified", D2D1::RectF(dateX, 59.0f, sizeX - 8.0f, 79.0f), rects.header, render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
        }
        if (sizeWidth > 0.0f) {
            drawTextClipped(render, L"Size", D2D1::RectF(sizeX, 59.0f, kindX - 8.0f, 79.0f), rects.header, render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
        }
        if (kindWidth > 0.0f) {
            drawTextClipped(render, L"Kind", D2D1::RectF(kindX, 59.0f, rects.header.right - padding, 79.0f), rects.header, render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
        }
    }

    drawTextClipped(
        render,
        L"Macintosh HD > Users > leo > home > environments > androidEnv > platform-tools",
        D2D1::RectF(rects.pathbar.left + 14.0f, rects.pathbar.top + 5.0f, rects.pathbar.right - 12.0f, rects.pathbar.bottom),
        window,
        render.textFormat(),
        D2D1::ColorF(0.34f, 0.34f, 0.34f));
}

}
