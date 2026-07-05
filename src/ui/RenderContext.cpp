#include "ui/RenderContext.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace finderx {

namespace {

constexpr float kMinFontSize = 11.0f;
constexpr float kMaxFontSize = 18.0f;

bool isPathCommand(char ch) {
    switch (ch) {
    case 'M':
    case 'm':
    case 'L':
    case 'l':
    case 'H':
    case 'h':
    case 'V':
    case 'v':
    case 'C':
    case 'c':
    case 'A':
    case 'a':
    case 'Z':
    case 'z':
        return true;
    default:
        return false;
    }
}

class SvgPathParser {
public:
    SvgPathParser(
        std::string_view input,
        ID2D1GeometrySink& sink,
        const D2D1_RECT_F& rect,
        float viewBoxWidth,
        float viewBoxHeight)
        : input_(input),
          sink_(sink) {
        const float width = rect.right - rect.left;
        const float height = rect.bottom - rect.top;
        scale_ = (std::min)(width / viewBoxWidth, height / viewBoxHeight);
        offsetX_ = rect.left + (width - viewBoxWidth * scale_) * 0.5f;
        offsetY_ = rect.top + (height - viewBoxHeight * scale_) * 0.5f;
    }

    bool parse() {
        char command = 0;
        while (true) {
            skipSeparators();
            if (atEnd()) {
                break;
            }
            if (isPathCommand(peek())) {
                command = input_[index_++];
            } else if (command == 0) {
                return false;
            }

            if (!parseCommand(command)) {
                return false;
            }
        }

        if (figureOpen_) {
            sink_.EndFigure(D2D1_FIGURE_END_OPEN);
            figureOpen_ = false;
        }
        return true;
    }

private:
    bool parseCommand(char command) {
        switch (command) {
        case 'M':
        case 'm':
            return parseMove(command == 'm');
        case 'L':
        case 'l':
            return parseLine(command == 'l');
        case 'H':
        case 'h':
            return parseHorizontal(command == 'h');
        case 'V':
        case 'v':
            return parseVertical(command == 'v');
        case 'C':
        case 'c':
            return parseCubic(command == 'c');
        case 'A':
        case 'a':
            return parseArc(command == 'a');
        case 'Z':
        case 'z':
            closeFigure();
            return true;
        default:
            return false;
        }
    }

    bool parseMove(bool relative) {
        float x = 0.0f;
        float y = 0.0f;
        if (!parseNumber(x) || !parseNumber(y)) {
            return false;
        }
        if (relative) {
            x += currentX_;
            y += currentY_;
        }
        beginFigure(x, y);

        while (hasNumberAhead()) {
            if (!parseNumber(x) || !parseNumber(y)) {
                return false;
            }
            if (relative) {
                x += currentX_;
                y += currentY_;
            }
            addLine(x, y);
        }
        return true;
    }

    bool parseLine(bool relative) {
        float x = 0.0f;
        float y = 0.0f;
        if (!hasNumberAhead()) {
            return false;
        }
        while (hasNumberAhead()) {
            if (!parseNumber(x) || !parseNumber(y)) {
                return false;
            }
            if (relative) {
                x += currentX_;
                y += currentY_;
            }
            addLine(x, y);
        }
        return true;
    }

    bool parseHorizontal(bool relative) {
        float x = 0.0f;
        if (!hasNumberAhead()) {
            return false;
        }
        while (hasNumberAhead()) {
            if (!parseNumber(x)) {
                return false;
            }
            addLine(relative ? currentX_ + x : x, currentY_);
        }
        return true;
    }

    bool parseVertical(bool relative) {
        float y = 0.0f;
        if (!hasNumberAhead()) {
            return false;
        }
        while (hasNumberAhead()) {
            if (!parseNumber(y)) {
                return false;
            }
            addLine(currentX_, relative ? currentY_ + y : y);
        }
        return true;
    }

    bool parseCubic(bool relative) {
        float x1 = 0.0f;
        float y1 = 0.0f;
        float x2 = 0.0f;
        float y2 = 0.0f;
        float x = 0.0f;
        float y = 0.0f;
        if (!hasNumberAhead()) {
            return false;
        }
        while (hasNumberAhead()) {
            if (!parseNumber(x1) || !parseNumber(y1) || !parseNumber(x2) || !parseNumber(y2) || !parseNumber(x) || !parseNumber(y)) {
                return false;
            }
            if (relative) {
                x1 += currentX_;
                y1 += currentY_;
                x2 += currentX_;
                y2 += currentY_;
                x += currentX_;
                y += currentY_;
            }
            sink_.AddBezier(D2D1::BezierSegment(mapPoint(x1, y1), mapPoint(x2, y2), mapPoint(x, y)));
            currentX_ = x;
            currentY_ = y;
        }
        return true;
    }

    bool parseArc(bool relative) {
        float rx = 0.0f;
        float ry = 0.0f;
        float rotation = 0.0f;
        float largeArc = 0.0f;
        float sweep = 0.0f;
        float x = 0.0f;
        float y = 0.0f;
        if (!hasNumberAhead()) {
            return false;
        }
        while (hasNumberAhead()) {
            if (!parseNumber(rx) || !parseNumber(ry) || !parseNumber(rotation) || !parseNumber(largeArc) || !parseNumber(sweep) || !parseNumber(x) || !parseNumber(y)) {
                return false;
            }
            if (relative) {
                x += currentX_;
                y += currentY_;
            }
            D2D1_ARC_SEGMENT segment{};
            segment.point = mapPoint(x, y);
            segment.size = D2D1::SizeF(std::abs(rx) * scale_, std::abs(ry) * scale_);
            segment.rotationAngle = rotation;
            segment.sweepDirection = sweep != 0.0f ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE;
            segment.arcSize = largeArc != 0.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL;
            sink_.AddArc(segment);
            currentX_ = x;
            currentY_ = y;
        }
        return true;
    }

    void beginFigure(float x, float y) {
        if (figureOpen_) {
            sink_.EndFigure(D2D1_FIGURE_END_OPEN);
        }
        currentX_ = x;
        currentY_ = y;
        startX_ = x;
        startY_ = y;
        sink_.BeginFigure(mapPoint(x, y), D2D1_FIGURE_BEGIN_FILLED);
        figureOpen_ = true;
    }

    void addLine(float x, float y) {
        sink_.AddLine(mapPoint(x, y));
        currentX_ = x;
        currentY_ = y;
    }

    void closeFigure() {
        if (!figureOpen_) {
            return;
        }
        sink_.EndFigure(D2D1_FIGURE_END_CLOSED);
        figureOpen_ = false;
        currentX_ = startX_;
        currentY_ = startY_;
    }

    D2D1_POINT_2F mapPoint(float x, float y) const {
        return D2D1::Point2F(offsetX_ + x * scale_, offsetY_ + y * scale_);
    }

    bool hasNumberAhead() {
        skipSeparators();
        if (atEnd()) {
            return false;
        }
        const char ch = peek();
        return ch == '-' || ch == '+' || ch == '.' || std::isdigit(static_cast<unsigned char>(ch));
    }

    bool parseNumber(float& value) {
        skipSeparators();
        if (atEnd()) {
            return false;
        }

        char* end = nullptr;
        value = std::strtof(input_.data() + index_, &end);
        if (end == input_.data() + index_) {
            return false;
        }
        index_ = static_cast<std::size_t>(end - input_.data());
        skipSeparators();
        return true;
    }

    void skipSeparators() {
        while (!atEnd()) {
            const char ch = peek();
            if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
                ++index_;
            } else {
                break;
            }
        }
    }

    bool atEnd() const {
        return index_ >= input_.size();
    }

    char peek() const {
        return input_[index_];
    }

    std::string_view input_;
    ID2D1GeometrySink& sink_;
    std::size_t index_ = 0;
    float scale_ = 1.0f;
    float offsetX_ = 0.0f;
    float offsetY_ = 0.0f;
    float currentX_ = 0.0f;
    float currentY_ = 0.0f;
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    bool figureOpen_ = false;
};

}

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
            fontFamily_.c_str(),
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize_,
            L"zh-CN",
            textFormat_.GetAddressOf()))) {
        return false;
    }

    const float headerFontSize = (std::max)(11.0f, fontSize_ - 1.0f);
    if (!headerTextFormat_ &&
        FAILED(dwriteFactory_->CreateTextFormat(
            fontFamily_.c_str(),
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            headerFontSize,
            L"zh-CN",
            headerTextFormat_.GetAddressOf()))) {
        return false;
    }

    textFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    headerTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    textFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    headerTextFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
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

bool RenderContext::setFontSize(float fontSize) {
    const float clamped = (std::clamp)(fontSize, kMinFontSize, kMaxFontSize);
    if (fontSize_ == clamped) {
        return true;
    }

    fontSize_ = clamped;
    headerTextFormat_.Reset();
    textFormat_.Reset();
    return createFactories();
}

bool RenderContext::setFontFamily(std::wstring fontFamily) {
    if (fontFamily.empty()) {
        fontFamily = L"Segoe UI";
    }
    if (fontFamily_ == fontFamily) {
        return true;
    }

    fontFamily_ = std::move(fontFamily);
    headerTextFormat_.Reset();
    textFormat_.Reset();
    return createFactories();
}

float RenderContext::fontSize() const {
    return fontSize_;
}

const std::wstring& RenderContext::fontFamily() const {
    return fontFamily_;
}

IDWriteTextFormat* RenderContext::textFormat() const {
    return textFormat_.Get();
}

IDWriteTextFormat* RenderContext::headerTextFormat() const {
    return headerTextFormat_.Get();
}

float RenderContext::measureTextWidth(std::wstring_view text, IDWriteTextFormat* format) const {
    if (!dwriteFactory_ || !format || text.empty()) {
        return 0.0f;
    }

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(dwriteFactory_->CreateTextLayout(text.data(),
                                                static_cast<UINT32>(text.size()),
                                                format,
                                                10000.0f,
                                                1000.0f,
                                                layout.GetAddressOf()))) {
        return 0.0f;
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics))) {
        return 0.0f;
    }
    return metrics.widthIncludingTrailingWhitespace;
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

void RenderContext::fillEllipse(const D2D1_ELLIPSE& ellipse, D2D1_COLOR_F color) {
    if (!target_ || !brush_ || ellipse.radiusX <= 0.0f || ellipse.radiusY <= 0.0f) {
        return;
    }

    brush_->SetColor(color);
    target_->FillEllipse(ellipse, brush_.Get());
}

void RenderContext::drawEllipse(const D2D1_ELLIPSE& ellipse, D2D1_COLOR_F color, float width) {
    if (!target_ || !brush_ || width <= 0.0f || ellipse.radiusX <= 0.0f || ellipse.radiusY <= 0.0f) {
        return;
    }

    brush_->SetColor(color);
    target_->DrawEllipse(ellipse, brush_.Get(), width);
}

void RenderContext::fillRect(const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    if (!target_ || !brush_ || rect.left >= rect.right || rect.top >= rect.bottom) {
        return;
    }

    brush_->SetColor(color);
    target_->FillRectangle(rect, brush_.Get());
}

void RenderContext::fillPolygon(std::span<const D2D1_POINT_2F> points, D2D1_COLOR_F color) {
    if (!target_ || !brush_ || !d2dFactory_ || points.size() < 3) {
        return;
    }

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(d2dFactory_->CreatePathGeometry(geometry.GetAddressOf()))) {
        return;
    }

    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(sink.GetAddressOf()))) {
        return;
    }

    sink->BeginFigure(points.front(), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLines(points.data() + 1, static_cast<UINT32>(points.size() - 1));
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    if (FAILED(sink->Close())) {
        return;
    }

    brush_->SetColor(color);
    target_->FillGeometry(geometry.Get(), brush_.Get());
}

bool RenderContext::fillSvgPath(
    std::string_view pathData,
    float viewBoxWidth,
    float viewBoxHeight,
    const D2D1_RECT_F& rect,
    D2D1_COLOR_F color) {
    if (!target_ || !brush_ || !d2dFactory_ || pathData.empty() || viewBoxWidth <= 0.0f || viewBoxHeight <= 0.0f || rect.left >= rect.right || rect.top >= rect.bottom) {
        return false;
    }

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(d2dFactory_->CreatePathGeometry(geometry.GetAddressOf()))) {
        return false;
    }

    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(sink.GetAddressOf()))) {
        return false;
    }

    SvgPathParser parser(pathData, *sink.Get(), rect, viewBoxWidth, viewBoxHeight);
    if (!parser.parse() || FAILED(sink->Close())) {
        return false;
    }

    brush_->SetColor(color);
    target_->FillGeometry(geometry.Get(), brush_.Get());
    return true;
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
