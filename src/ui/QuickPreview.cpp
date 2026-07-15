#include "ui/QuickPreview.h"

#include "ui/ThemeTokens.h"

#include <algorithm>
#include <cstddef>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string_view>

namespace finderx::ui {

namespace {

constexpr std::uintmax_t kMaxTextPreviewBytes = 1024 * 1024;
constexpr UINT kMaxImageDimension = 8192;

std::wstring lowercase(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return value;
}

std::wstring fileNameFromPath(const std::wstring& path) {
    return std::filesystem::path(path).filename().wstring();
}

std::wstring extensionForPath(const std::wstring& path) {
    return lowercase(std::filesystem::path(path).extension().wstring());
}

bool matchesExtension(const std::wstring& path, std::span<const std::wstring_view> extensions) {
    const std::wstring extension = extensionForPath(path);
    for (std::wstring_view candidate : extensions) {
        if (extension == candidate) {
            return true;
        }
    }
    return false;
}

std::wstring utf8ToWide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::wstring ansiToWide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::wstring bytesToText(const std::vector<char>& bytes) {
    if (bytes.size() >= 2) {
        const auto b0 = static_cast<unsigned char>(bytes[0]);
        const auto b1 = static_cast<unsigned char>(bytes[1]);
        if (b0 == 0xFF && b1 == 0xFE) {
            const std::size_t charCount = (bytes.size() - 2) / sizeof(wchar_t);
            return std::wstring(reinterpret_cast<const wchar_t*>(bytes.data() + 2), charCount);
        }
        if (b0 == 0xFE && b1 == 0xFF) {
            std::wstring result;
            result.reserve((bytes.size() - 2) / 2);
            for (std::size_t i = 2; i + 1 < bytes.size(); i += 2) {
                const wchar_t ch = static_cast<wchar_t>(
                    (static_cast<unsigned char>(bytes[i]) << 8) | static_cast<unsigned char>(bytes[i + 1]));
                result.push_back(ch);
            }
            return result;
        }
    }

    std::string_view text(bytes.data(), bytes.size());
    if (bytes.size() >= 3
        && static_cast<unsigned char>(bytes[0]) == 0xEF
        && static_cast<unsigned char>(bytes[1]) == 0xBB
        && static_cast<unsigned char>(bytes[2]) == 0xBF) {
        text.remove_prefix(3);
    }

    std::wstring utf8 = utf8ToWide(text);
    return utf8.empty() && !text.empty() ? ansiToWide(text) : utf8;
}

QuickPreviewContent errorContent(const std::wstring& path, std::wstring message) {
    QuickPreviewContent content;
    content.kind = QuickPreviewKind::Error;
    content.path = path;
    content.title = fileNameFromPath(path);
    content.message = std::move(message);
    return content;
}

QuickPreviewContent loadTextPreview(const std::wstring& path) {
    QuickPreviewContent content;
    content.kind = QuickPreviewKind::Text;
    content.path = path;
    content.title = fileNameFromPath(path);

    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    if (error) {
        return errorContent(path, L"Cannot read this file");
    }

    const std::size_t byteCount = static_cast<std::size_t>((std::min)(size, kMaxTextPreviewBytes));
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return errorContent(path, L"Cannot read this file");
    }

    std::vector<char> bytes(byteCount);
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    bytes.resize(static_cast<std::size_t>(input.gcount()));
    content.text = bytesToText(bytes);
    if (size > kMaxTextPreviewBytes) {
        content.message = L"Preview truncated";
    }
    return content;
}

QuickPreviewContent loadImagePreview(const std::wstring& path) {
    QuickPreviewContent content;
    content.kind = QuickPreviewKind::Image;
    content.path = path;
    content.title = fileNameFromPath(path);

    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
        return errorContent(path, L"Cannot initialize image preview");
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder))) {
        return errorContent(path, L"Cannot decode this image");
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return errorContent(path, L"Cannot decode this image");
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0
        || width > kMaxImageDimension || height > kMaxImageDimension) {
        return errorContent(path, L"Image is too large to preview");
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter))
        || FAILED(converter->Initialize(frame.Get(),
                                        GUID_WICPixelFormat32bppPBGRA,
                                        WICBitmapDitherTypeNone,
                                        nullptr,
                                        0.0,
                                        WICBitmapPaletteTypeMedianCut))) {
        return errorContent(path, L"Cannot decode this image");
    }

    const UINT stride = width * 4;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(stride) * height);
    if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(pixels.size()), pixels.data()))) {
        return errorContent(path, L"Cannot decode this image");
    }

    content.imageWidth = width;
    content.imageHeight = height;
    content.imagePixels = std::move(pixels);
    return content;
}

void drawCenteredText(RenderContext& render, std::wstring_view text, const D2D1_RECT_F& rect, D2D1_COLOR_F color) {
    render.drawText(text, rect, render.textFormat(), color);
}

} // namespace

QuickPreviewLayout quickPreviewLayout(D2D1_SIZE_F size, D2D1_POINT_2F offset) {
    constexpr float margin = 52.0f;
    const float width = (std::min)(size.width - margin * 2.0f, 760.0f);
    const float height = (std::min)(size.height - margin * 2.0f, 560.0f);

    QuickPreviewLayout layout;
    if (width <= 220.0f || height <= 160.0f) {
        return layout;
    }

    const float minLeft = 8.0f;
    const float minTop = 8.0f;
    const float maxLeft = (std::max)(minLeft, size.width - width - 8.0f);
    const float maxTop = (std::max)(minTop, size.height - height - 8.0f);
    const float left = (std::clamp)((size.width - width) * 0.5f + offset.x, minLeft, maxLeft);
    const float top = (std::clamp)((size.height - height) * 0.5f + offset.y, minTop, maxTop);
    layout.visible = true;
    layout.shell = D2D1::RectF(left, top, left + width, top + height);
    layout.header = D2D1::RectF(layout.shell.left, layout.shell.top, layout.shell.right, layout.shell.top + 44.0f);
    layout.body = D2D1::RectF(
        layout.shell.left + 18.0f,
        layout.shell.top + 58.0f,
        layout.shell.right - 18.0f,
        layout.shell.bottom - 18.0f);
    layout.textViewport = D2D1::RectF(
        layout.body.left + 14.0f,
        layout.body.top + 12.0f,
        layout.body.right - 14.0f,
        layout.body.bottom - 12.0f);
    return layout;
}

bool isQuickPreviewModalMouseMessage(UINT message) {
    switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
    case WM_MOUSEMOVE:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_CONTEXTMENU:
        return true;
    default:
        return false;
    }
}

bool isQuickPreviewModalKeyboardMessage(UINT message) {
    switch (message) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
        return true;
    default:
        return false;
    }
}

bool isQuickPreviewSystemKeyboardMessage(UINT message) {
    switch (message) {
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
        return true;
    default:
        return false;
    }
}

bool isQuickPreviewCloseKey(WPARAM key) {
    return key == VK_SPACE || key == VK_ESCAPE;
}

bool isQuickPreviewTextExtension(const std::wstring& path) {
    static constexpr std::wstring_view kExtensions[] = {
        L".txt", L".log", L".md", L".json", L".xml", L".ini", L".cfg", L".csv",
        L".cpp", L".c", L".h", L".hpp", L".cs", L".js", L".ts", L".css", L".html", L".py", L".ps1", L".bat"
    };
    return matchesExtension(path, kExtensions);
}

bool isQuickPreviewImageExtension(const std::wstring& path) {
    static constexpr std::wstring_view kExtensions[] = {
        L".png", L".jpg", L".jpeg", L".bmp", L".gif", L".tif", L".tiff", L".webp"
    };
    return matchesExtension(path, kExtensions);
}

QuickPreviewContent loadQuickPreviewContent(const std::wstring& path) {
    if (path.empty()) {
        return errorContent(path, L"No file selected");
    }

    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return errorContent(path, L"Preview is available for files");
    }

    if (isQuickPreviewTextExtension(path)) {
        return loadTextPreview(path);
    }
    if (isQuickPreviewImageExtension(path)) {
        return loadImagePreview(path);
    }

    QuickPreviewContent content;
    content.kind = QuickPreviewKind::Unsupported;
    content.path = path;
    content.title = fileNameFromPath(path);
    content.message = L"Preview is not available for this file type";
    return content;
}

void drawQuickPreview(RenderContext& render, QuickPreviewContent& content, ThemeMode themeMode, D2D1_POINT_2F offset) {
    const D2D1_SIZE_F size = render.sizeDips();
    const QuickPreviewLayout layout = quickPreviewLayout(size, offset);
    if (!layout.visible) {
        return;
    }
    const D2D1_RECT_F& shell = layout.shell;
    const D2D1_RECT_F& header = layout.header;
    const D2D1_RECT_F& body = layout.body;

    const ThemeTokens tokens = themeTokens(themeMode);

    render.fillRect(D2D1::RectF(0.0f, 0.0f, size.width, size.height), D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.34f));
    render.fillRoundedRect(D2D1::RoundedRect(shell, 10.0f, 10.0f), tokens.appOverlay);
    render.drawRoundedRect(D2D1::RoundedRect(shell, 10.0f, 10.0f), tokens.appLine, 1.0f);
    render.drawLine(D2D1::Point2F(header.left + 1.0f, header.bottom), D2D1::Point2F(header.right - 1.0f, header.bottom), tokens.appLine, 1.0f);
    render.drawText(content.title, D2D1::RectF(header.left + 18.0f, header.top + 10.0f, header.right - 18.0f, header.bottom), render.headerTextFormat(), tokens.ink);

    if (content.kind == QuickPreviewKind::Text) {
        render.fillRoundedRect(D2D1::RoundedRect(body, 6.0f, 6.0f), tokens.app);
        render.drawRoundedRect(D2D1::RoundedRect(body, 6.0f, 6.0f), tokens.appLine, 1.0f);
        if (!content.message.empty()) {
            render.drawText(content.message, D2D1::RectF(body.left + 14.0f, body.bottom - 30.0f, body.right - 14.0f, body.bottom - 8.0f), render.headerTextFormat(), tokens.inkFaint);
        }
        return;
    }

    if (content.kind == QuickPreviewKind::Image && !content.imagePixels.empty() && content.imageWidth > 0 && content.imageHeight > 0 && render.target()) {
        Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
        const D2D1_BITMAP_PROPERTIES properties = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        const UINT stride = content.imageWidth * 4;
        if (SUCCEEDED(render.target()->CreateBitmap(
                D2D1::SizeU(content.imageWidth, content.imageHeight),
                content.imagePixels.data(),
                stride,
                properties,
                bitmap.GetAddressOf()))) {
            const float scale = (std::min)(
                (body.right - body.left) / static_cast<float>(content.imageWidth),
                (body.bottom - body.top) / static_cast<float>(content.imageHeight));
            const float imageWidth = static_cast<float>(content.imageWidth) * scale;
            const float imageHeight = static_cast<float>(content.imageHeight) * scale;
            const D2D1_RECT_F imageRect = D2D1::RectF(
                body.left + ((body.right - body.left) - imageWidth) * 0.5f,
                body.top + ((body.bottom - body.top) - imageHeight) * 0.5f,
                body.left + ((body.right - body.left) + imageWidth) * 0.5f,
                body.top + ((body.bottom - body.top) + imageHeight) * 0.5f);
            render.target()->DrawBitmap(bitmap.Get(), imageRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            return;
        }
    }

    drawCenteredText(render, content.message.empty() ? L"Cannot preview this file" : content.message, body, tokens.inkDull);
}

} // namespace finderx::ui
