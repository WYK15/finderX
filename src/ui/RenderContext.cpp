#include "ui/RenderContext.h"

namespace finderx {

bool RenderContext::initialize(HWND hwnd) {
    return createFactories() && createDeviceResources(hwnd);
}

bool RenderContext::createFactories() {
    if (!d2dFactory_ &&
        FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.GetAddressOf()))) {
        return false;
    }

    if (!dwriteFactory_ &&
        FAILED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf())))) {
        return false;
    }

    if (!textFormat_ &&
        FAILED(dwriteFactory_->CreateTextFormat(
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

    if (!headerTextFormat_ &&
        FAILED(dwriteFactory_->CreateTextFormat(
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

bool RenderContext::createDeviceResources(HWND hwnd) {
    if (target_) {
        return true;
    }

    RECT client{};
    GetClientRect(hwnd, &client);
    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT32>(client.right - client.left),
        static_cast<UINT32>(client.bottom - client.top));

    if (FAILED(d2dFactory_->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            target_.GetAddressOf()))) {
        return false;
    }

    return SUCCEEDED(target_->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::Black),
        brush_.GetAddressOf()));
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
    const HRESULT result = target_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        brush_.Reset();
        target_.Reset();
    }
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

void RenderContext::drawText(
    std::wstring_view text,
    const D2D1_RECT_F& rect,
    IDWriteTextFormat* format,
    D2D1_COLOR_F color) {
    brush_->SetColor(color);
    target_->DrawTextW(
        text.data(),
        static_cast<UINT32>(text.size()),
        format,
        rect,
        brush_.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
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
