#include "ui/RenderContext.h"

#include <cstdlib>
#include <iostream>

namespace {

constexpr wchar_t kWindowClassName[] = L"FinderXRenderContextTestWindow";

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

HWND createHiddenWindow() {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = DefWindowProcW;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kWindowClassName;
    RegisterClassW(&windowClass);

    return CreateWindowExW(0,
                           kWindowClassName,
                           L"RenderContextTests",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           400,
                           300,
                           nullptr,
                           nullptr,
                           GetModuleHandleW(nullptr),
                           nullptr);
}

} // namespace

int main() {
    using finderx::RenderContext;

    HWND window = createHiddenWindow();
    require(window != nullptr, "hidden test window should be created");

    RenderContext render;
    require(render.initialize(window), "render context should initialize");

    const float narrow = render.measureTextWidth(L"iiiiiiiiii", render.textFormat());
    const float wide = render.measureTextWidth(L"WWWWWWWWWW", render.textFormat());
    require(narrow > 0.0f, "narrow text should have measurable width");
    require(wide > narrow * 1.5f, "text measurement should use real glyph widths, not fixed estimates");

    DestroyWindow(window);
    std::cout << "RenderContextTests passed\n";
    return 0;
}
