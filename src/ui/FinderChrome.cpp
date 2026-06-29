#include "ui/FinderChrome.h"

#include <algorithm>

namespace finderx {

namespace {

constexpr float kSidebarWidth = 174.0f;
constexpr float kToolbarHeight = 53.0f;
constexpr float kHeaderBottom = 81.0f;
constexpr float kPathbarHeight = 28.0f;

D2D1_COLOR_F rgb(float value) {
    return D2D1::ColorF(value, value, value);
}

void drawSeparator(RenderContext& render, float x, float top, float bottom) {
    render.drawLine(
        D2D1::Point2F(x, top),
        D2D1::Point2F(x, bottom),
        D2D1::ColorF(0.82f, 0.82f, 0.82f));
}

}

LayoutRects FinderChrome::layout(float width, float height) const {
    const float right = (std::max)(width, kSidebarWidth);
    const float bottom = (std::max)(height, kHeaderBottom + kPathbarHeight);
    const float contentLeft = (std::min)(kSidebarWidth, right);
    const float pathbarTop = (std::max)(kHeaderBottom, bottom - kPathbarHeight);

    LayoutRects rects;
    rects.sidebar = D2D1::RectF(0.0f, 0.0f, contentLeft, bottom);
    rects.toolbar = D2D1::RectF(contentLeft, 0.0f, right, kToolbarHeight);
    rects.header = D2D1::RectF(contentLeft, kToolbarHeight, right, kHeaderBottom);
    rects.pathbar = D2D1::RectF(contentLeft, pathbarTop, right, bottom);
    rects.list = D2D1::RectF(contentLeft, kHeaderBottom, right, pathbarTop);
    return rects;
}

void FinderChrome::draw(RenderContext& render, const LayoutRects& rects) {
    render.fillRect(rects.sidebar, D2D1::ColorF(0.92f, 0.93f, 0.94f));
    render.fillRect(rects.toolbar, D2D1::ColorF(0.97f, 0.97f, 0.965f));
    render.fillRect(rects.header, D2D1::ColorF(0.995f, 0.995f, 0.995f));
    render.fillRect(rects.pathbar, D2D1::ColorF(0.975f, 0.975f, 0.972f));

    drawSeparator(render, rects.sidebar.right, 0.0f, rects.sidebar.bottom);
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

    render.drawText(
        L"FAVORITES",
        D2D1::RectF(18.0f, 70.0f, 150.0f, 91.0f),
        render.headerTextFormat(),
        D2D1::ColorF(0.50f, 0.50f, 0.50f));
    render.drawText(
        L"Applications\nDocuments\nDesktop\nHome\nDownloads",
        D2D1::RectF(34.0f, 94.0f, 160.0f, 220.0f),
        render.textFormat(),
        D2D1::ColorF(0.16f, 0.16f, 0.16f));
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(24.0f, 174.0f, 164.0f, 200.0f), 6.0f, 6.0f),
        D2D1::ColorF(0.80f, 0.82f, 0.84f));
    render.drawText(
        L"Home",
        D2D1::RectF(54.0f, 178.0f, 150.0f, 198.0f),
        render.textFormat(),
        D2D1::ColorF(0.10f, 0.10f, 0.10f));

    render.drawText(
        L"\u2039   \u203a",
        D2D1::RectF(rects.toolbar.left + 22.0f, 15.0f, rects.toolbar.left + 92.0f, 43.0f),
        render.textFormat(),
        D2D1::ColorF(0.56f, 0.56f, 0.56f));
    render.drawText(
        L"home",
        D2D1::RectF(rects.toolbar.left + 106.0f, 16.0f, rects.toolbar.left + 240.0f, 45.0f),
        render.headerTextFormat(),
        D2D1::ColorF(0.18f, 0.18f, 0.18f));
    render.drawText(
        L"\u25A6   \u25A4   \u25A1   \u2022\u2022\u2022",
        D2D1::RectF(rects.toolbar.left + 380.0f, 17.0f, rects.toolbar.right - 232.0f, 44.0f),
        render.textFormat(),
        D2D1::ColorF(0.36f, 0.36f, 0.36f));

    const float searchLeft = (std::max)(rects.toolbar.left + 420.0f, rects.toolbar.right - 202.0f);
    render.fillRoundedRect(
        D2D1::RoundedRect(D2D1::RectF(searchLeft, 16.0f, rects.toolbar.right - 14.0f, 44.0f), 7.0f, 7.0f),
        D2D1::ColorF(1.0f, 1.0f, 1.0f));
    render.drawLine(
        D2D1::Point2F(searchLeft, 16.0f),
        D2D1::Point2F(rects.toolbar.right - 14.0f, 16.0f),
        rgb(0.88f));
    render.drawText(
        L"\u2315 Search",
        D2D1::RectF(searchLeft + 11.0f, 20.0f, rects.toolbar.right - 26.0f, 43.0f),
        render.textFormat(),
        D2D1::ColorF(0.53f, 0.53f, 0.53f));

    const float nameX = rects.header.left + 64.0f;
    const float dateX = rects.header.right - 390.0f;
    const float sizeX = rects.header.right - 230.0f;
    const float kindX = rects.header.right - 130.0f;
    render.drawText(L"Name", D2D1::RectF(nameX, 59.0f, dateX - 8.0f, 79.0f), render.headerTextFormat(), D2D1::ColorF(0.18f, 0.18f, 0.18f));
    render.drawText(L"Date Modified", D2D1::RectF(dateX, 59.0f, sizeX - 8.0f, 79.0f), render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
    render.drawText(L"Size", D2D1::RectF(sizeX, 59.0f, kindX - 8.0f, 79.0f), render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));
    render.drawText(L"Kind", D2D1::RectF(kindX, 59.0f, rects.header.right - 12.0f, 79.0f), render.headerTextFormat(), D2D1::ColorF(0.38f, 0.38f, 0.38f));

    render.drawText(
        L"Macintosh HD > Users > leo > home > environments > androidEnv > platform-tools",
        D2D1::RectF(rects.pathbar.left + 14.0f, rects.pathbar.top + 5.0f, rects.pathbar.right - 12.0f, rects.pathbar.bottom),
        render.textFormat(),
        D2D1::ColorF(0.34f, 0.34f, 0.34f));
}

}
