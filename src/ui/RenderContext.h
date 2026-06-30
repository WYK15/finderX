#pragma once

#include <windows.h>

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <string_view>

namespace finderx {

class RenderContext {
public:
    bool initialize(HWND hwnd);
    bool isReady() const;
    bool resize(UINT width, UINT height);
    void beginDraw();
    bool endDraw();
    void clear(D2D1_COLOR_F color);

    ID2D1HwndRenderTarget* target() const;
    IDWriteTextFormat* textFormat() const;
    IDWriteTextFormat* headerTextFormat() const;

    void drawText(std::wstring_view text, const D2D1_RECT_F& rect, IDWriteTextFormat* format, D2D1_COLOR_F color);
    void fillRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color);
    void drawRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color, float width = 1.0f);
    void fillRect(const D2D1_RECT_F& rect, D2D1_COLOR_F color);
    void drawLine(D2D1_POINT_2F start, D2D1_POINT_2F end, D2D1_COLOR_F color, float width = 1.0f);

private:
    bool createFactories();
    bool createDeviceResources(HWND hwnd);
    void resetAllResources();
    void resetDeviceResources();

    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat_;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> headerTextFormat_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;
};

}
