#include "ui/QuickPreview.h"

#include <d2d1helper.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    require(ui::isQuickPreviewTextExtension(L"C:\\Temp\\readme.TXT"), "txt should be text-previewable");
    require(ui::isQuickPreviewTextExtension(L"C:\\Temp\\main.cpp"), "cpp should be text-previewable");
    require(ui::isQuickPreviewImageExtension(L"C:\\Temp\\photo.PNG"), "png should be image-previewable");
    require(ui::isQuickPreviewImageExtension(L"C:\\Temp\\photo.jpeg"), "jpeg should be image-previewable");
    require(!ui::isQuickPreviewTextExtension(L"C:\\Temp\\photo.png"), "png should not be text-previewable");

    const std::filesystem::path tempPath = std::filesystem::temp_directory_path() / L"finderx_quick_preview_test.txt";
    {
        std::ofstream output(tempPath, std::ios::binary);
        output << "\xEF\xBB\xBFHello preview";
    }

    const ui::QuickPreviewContent content = ui::loadQuickPreviewContent(tempPath.wstring());
    require(content.kind == ui::QuickPreviewKind::Text, "text file should load as text preview");
    require(content.text == L"Hello preview", "utf-8 bom text should decode");

    std::error_code ignored;
    std::filesystem::remove(tempPath, ignored);

    const ui::QuickPreviewLayout layout = ui::quickPreviewLayout(D2D1::SizeF(1200.0f, 800.0f));
    require(layout.visible, "large windows should produce a visible preview layout");
    require(layout.shell.left < layout.shell.right && layout.shell.top < layout.shell.bottom,
            "preview shell should have positive area");
    require(layout.textViewport.left > layout.body.left
            && layout.textViewport.top > layout.body.top
            && layout.textViewport.right < layout.body.right
            && layout.textViewport.bottom < layout.body.bottom,
            "text viewport should be inset inside the preview body");
    const ui::QuickPreviewLayout movedLayout = ui::quickPreviewLayout(
        D2D1::SizeF(1200.0f, 800.0f),
        D2D1::Point2F(80.0f, 40.0f));
    require(movedLayout.shell.left > layout.shell.left && movedLayout.shell.top > layout.shell.top,
            "preview layout offset should move the shell");
    const ui::QuickPreviewLayout clampedLayout = ui::quickPreviewLayout(
        D2D1::SizeF(1200.0f, 800.0f),
        D2D1::Point2F(10000.0f, 10000.0f));
    require(clampedLayout.shell.right <= 1192.0f && clampedLayout.shell.bottom <= 792.0f,
            "preview layout offset should clamp the shell inside the window");
    require(!ui::quickPreviewLayout(D2D1::SizeF(200.0f, 140.0f)).visible,
            "undersized windows should hide preview content geometry");

    require(ui::isQuickPreviewModalMouseMessage(WM_LBUTTONDOWN),
            "preview should consume background left clicks");
    require(ui::isQuickPreviewModalMouseMessage(WM_RBUTTONDOWN),
            "preview should consume background right clicks");
    require(ui::isQuickPreviewModalMouseMessage(WM_MOUSEWHEEL),
            "preview should consume background wheel input");
    require(!ui::isQuickPreviewModalMouseMessage(WM_PAINT),
            "preview should not consume paint messages");

    struct KeyboardMessageCase {
        UINT message;
        const char* failureMessage;
    };
    constexpr std::array<KeyboardMessageCase, 8> keyboardMessages{{
        {WM_KEYDOWN, "preview should consume key-down messages"},
        {WM_KEYUP, "preview should consume key-up messages"},
        {WM_CHAR, "preview should consume character messages"},
        {WM_DEADCHAR, "preview should consume dead-character messages"},
        {WM_SYSKEYDOWN, "preview should consume system key-down messages"},
        {WM_SYSKEYUP, "preview should consume system key-up messages"},
        {WM_SYSCHAR, "preview should consume system character messages"},
        {WM_SYSDEADCHAR, "preview should consume system dead-character messages"},
    }};
    for (const KeyboardMessageCase& testCase : keyboardMessages) {
        require(ui::isQuickPreviewModalKeyboardMessage(testCase.message), testCase.failureMessage);
    }
    require(!ui::isQuickPreviewModalKeyboardMessage(WM_PAINT),
            "preview keyboard guard should not consume paint messages");
    require(!ui::isQuickPreviewModalKeyboardMessage(WM_MOUSEMOVE),
            "preview keyboard guard should not consume mouse movement");

    constexpr std::array<KeyboardMessageCase, 4> systemKeyboardMessages{{
        {WM_SYSKEYDOWN, "text preview should consume system key-down messages"},
        {WM_SYSKEYUP, "text preview should consume system key-up messages"},
        {WM_SYSCHAR, "text preview should consume system character messages"},
        {WM_SYSDEADCHAR, "text preview should consume system dead-character messages"},
    }};
    for (const KeyboardMessageCase& testCase : systemKeyboardMessages) {
        require(ui::isQuickPreviewSystemKeyboardMessage(testCase.message), testCase.failureMessage);
    }
    require(!ui::isQuickPreviewSystemKeyboardMessage(WM_KEYDOWN),
            "text preview system-key guard should preserve ordinary key-down messages");
    require(!ui::isQuickPreviewSystemKeyboardMessage(WM_CHAR),
            "text preview system-key guard should preserve ordinary character messages");

    require(ui::isQuickPreviewCloseKey(VK_SPACE), "Space should close preview");
    require(ui::isQuickPreviewCloseKey(VK_ESCAPE), "Escape should close preview");
    require(!ui::isQuickPreviewCloseKey(VK_DOWN), "text navigation should remain available");

    std::cout << "QuickPreviewTests passed\n";
    return 0;
}
