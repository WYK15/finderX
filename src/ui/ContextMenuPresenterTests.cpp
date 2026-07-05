#include "ui/ContextMenuPresenter.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;
using namespace finderx::ui;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

bool differs(D2D1_COLOR_F left, D2D1_COLOR_F right) {
    return left.r != right.r || left.g != right.g || left.b != right.b || left.a != right.a;
}

void testMetricsScaleWithFontSize() {
    const ContextMenuMetrics compactMetrics = contextMenuMetrics(12.0f);
    const ContextMenuMetrics largeMetrics = contextMenuMetrics(18.0f);

    require(compactMetrics.itemHeight >= 28, "small menu row should keep a usable hit target");
    require(largeMetrics.itemHeight > compactMetrics.itemHeight, "larger fonts should produce taller menu rows");
    require(largeMetrics.minWidth >= compactMetrics.minWidth, "larger fonts should not shrink menu width");
}

void testHoverColorIsThemeAwareAndVisible() {
    const D2D1_COLOR_F lightHover = contextMenuHoverColor(ThemeMode::Light);
    const D2D1_COLOR_F darkHover = contextMenuHoverColor(ThemeMode::Dark);
    const ThemeTokens light = themeTokens(ThemeMode::Light);
    const ThemeTokens dark = themeTokens(ThemeMode::Dark);

    require(differs(lightHover, light.menu), "light hover should differ from menu background");
    require(differs(darkHover, dark.menu), "dark hover should differ from menu background");
    require(lightHover.a == 1.0f, "light hover should be opaque");
    require(darkHover.a == 1.0f, "dark hover should be opaque");
}

void testSelectedTextUsesReadableColor() {
    const D2D1_COLOR_F selectedLight = contextMenuTextColor(ThemeMode::Light, true);
    const D2D1_COLOR_F selectedDark = contextMenuTextColor(ThemeMode::Dark, true);

    require(selectedLight.a == 1.0f, "selected light text should remain opaque");
    require(selectedDark.a == 1.0f, "selected dark text should remain opaque");
    require(selectedDark.r > 0.75f && selectedDark.g > 0.75f && selectedDark.b > 0.75f,
        "selected dark text should stay bright on hover");
}

} // namespace

int main() {
    testMetricsScaleWithFontSize();
    testHoverColorIsThemeAwareAndVisible();
    testSelectedTextUsesReadableColor();
    std::cout << "ContextMenuPresenterTests passed\n";
}
