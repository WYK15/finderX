#include "ui/RenderContext.h"

namespace finderx {

bool RenderContext::initialize(HWND hwnd) {
    if (isReady()) {
        return true;
    }

    if (!createFactories() || !createDeviceResources(hwnd) || !isReady()) {
        resetAllResources();
        return false;
    }

    return true;
}

bool RenderContext::isReady() const {
    return d2dFactory_ && dwriteFactory_ && target_ && textFormat_ && headerTextFormat_ && brush_;
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
    if (target_ && brush_) {
        return true;
    }

    resetDeviceResources();

    RECT client{};
    GetClientRect(hwnd, &client);
    const D2D1_SIZE_U size = D2D1::SizeU(
        static_cast<UINT32>(client.right - client.left),
        static_cast<UINT32>(client.bottom - client.top));

    if (FAILED(d2dFactory_->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            target_.GetAddressOf()))) {
        resetDeviceResources();
        return false;
    }

    if (FAILED(target_->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::Black),
            brush_.GetAddressOf()))) {
        resetDeviceResources();
        return false;
    }

    return true;
}

bool RenderContext::resize(UINT width, UINT height) {
    if (!target_) {
        return true;
    }

    const HRESULT result = target_->Resize(D2D1::SizeU(width, height));
    if (result == D2DERR_RECREATE_TARGET || FAILED(result)) {
        resetDeviceResources();
        return false;
    }

    return true;
}

void RenderContext::beginDraw() {
    target_->BeginDraw();
}

bool RenderContext::endDraw() {
    const HRESULT result = target_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET || FAILED(result)) {
        resetDeviceResources();
        return true;
    }
    return false;
}

void RenderContext::clear(D2D1_COLOR_F color) {
    target_->Clear(color);
}

ID2D1HwndRenderTarget* RenderContext::target() const {
    return target_.Get();
}

DpiScale RenderContext::dpiScale() const {
    DpiScale dpi;
    if (target_) {
        target_->GetDpi(&dpi.x, &dpi.y);
    }
    return dpi;
}

D2D1_SIZE_F RenderContext::sizeDips() const {
    if (!target_) {
        return D2D1::SizeF();
    }
    return target_->GetSize();
}

D2D1_POINT_2F RenderContext::clientPointToDips(POINT point) const {
    return pixelsToDips(static_cast<int>(point.x), static_cast<int>(point.y), dpiScale());
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
    if (!target_ || !brush_ || !format || text.empty() || rect.left >= rect.right || rect.top >= rect.bottom) {
        return;
    }

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
    if (!target_ || !brush_ || rect.rect.left >= rect.rect.right || rect.rect.top >= rect.rect.bottom) {
        return;
    }

    brush_->SetColor(color);
    target_->FillRoundedRectangle(rect, brush_.Get());
}

void RenderContext::drawRoundedRect(const D2D1_ROUNDED_RECT& rect, D2D1_COLOR_F color, float width) {
    if (!target_ || !brush_ || width <= 0.0f || rect.rect.left >= rect.rect.right || rect.rect.top >= rect.rect.bottom) {
        return;
    }

    brush_->SetColor(color);
    target_->DrawRoundedRectangle(rect, brush_.Get(), width);
}

void RenderContext::fillRect(const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    if (!target_ || !brush_ || rect.left >= rect.right || rect.top >= rect.bottom) {
        return;
    }

    brush_->SetColor(color);
    target_->FillRectangle(rect, brush_.Get());
}

void RenderContext::drawLine(D2D1_POINT_2F start, D2D1_POINT_2F end, D2D1_COLOR_F color, float width) {
    if (!target_ || !brush_ || width <= 0.0f) {
        return;
    }

    brush_->SetColor(color);
    target_->DrawLine(start, end, brush_.Get(), width);
}

void RenderContext::resetAllResources() {
    resetDeviceResources();
    headerTextFormat_.Reset();
    textFormat_.Reset();
    dwriteFactory_.Reset();
    d2dFactory_.Reset();
}

void RenderContext::resetDeviceResources() {
    brush_.Reset();
    target_.Reset();
}

}
