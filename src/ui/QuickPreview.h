#pragma once

#include "settings/AppSettings.h"
#include "ui/RenderContext.h"

#include <d2d1.h>
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstdint>
#include <string>
#include <vector>

namespace finderx::ui {

enum class QuickPreviewKind {
    None,
    Text,
    Image,
    Unsupported,
    Error
};

struct QuickPreviewContent {
    QuickPreviewKind kind = QuickPreviewKind::None;
    std::wstring path;
    std::wstring title;
    std::wstring message;
    std::wstring text;
    std::vector<std::uint8_t> imagePixels;
    UINT imageWidth = 0;
    UINT imageHeight = 0;
};

struct QuickPreviewLayout {
    bool visible = false;
    D2D1_RECT_F shell{};
    D2D1_RECT_F header{};
    D2D1_RECT_F body{};
    D2D1_RECT_F textViewport{};
};

QuickPreviewLayout quickPreviewLayout(D2D1_SIZE_F size, D2D1_POINT_2F offset = D2D1_POINT_2F{});
bool isQuickPreviewModalMouseMessage(UINT message);
bool isQuickPreviewModalKeyboardMessage(UINT message);
bool isQuickPreviewSystemKeyboardMessage(UINT message);
bool isQuickPreviewCloseKey(WPARAM key);
bool isQuickPreviewTextExtension(const std::wstring& path);
bool isQuickPreviewImageExtension(const std::wstring& path);
QuickPreviewContent loadQuickPreviewContent(const std::wstring& path);
void drawQuickPreview(RenderContext& render, QuickPreviewContent& content, ThemeMode themeMode, D2D1_POINT_2F offset = D2D1_POINT_2F{});

} // namespace finderx::ui
