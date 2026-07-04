#include "ui/ThemeTokens.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

bool differs(D2D1_COLOR_F a, D2D1_COLOR_F b) {
    return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
}

void testThemeTokensExposeDistinctLightAndDarkSurfaces() {
    const ThemeTokens light = themeTokens(ThemeMode::Light);
    const ThemeTokens dark = themeTokens(ThemeMode::Dark);

    require(differs(light.app, dark.app), "app surface should differ between light and dark themes");
    require(differs(light.sidebar, dark.sidebar), "sidebar surface should differ between light and dark themes");
    require(differs(light.menu, dark.menu), "menu surface should differ between light and dark themes");
    require(differs(light.rowSelected, dark.rowSelected), "selected row color should differ between themes");
}

void testAccentStaysBlueAcrossThemes() {
    const ThemeTokens light = themeTokens(ThemeMode::Light);
    const ThemeTokens dark = themeTokens(ThemeMode::Dark);

    require(light.accent.b > light.accent.r, "light accent should stay blue");
    require(light.accent.b > light.accent.g * 0.70f, "light accent should retain strong blue component");
    require(dark.accent.b > dark.accent.r, "dark accent should stay blue");
    require(dark.accent.b > dark.accent.g * 0.70f, "dark accent should retain strong blue component");
}

void testRadiiMatchDesignBaseline() {
    const ThemeTokens tokens = themeTokens(ThemeMode::Dark);

    require(tokens.radiusControl == 6.0f, "control radius should be 6px");
    require(tokens.radiusPanel == 8.0f, "panel radius should be 8px");
    require(tokens.radiusWindow == 10.0f, "window radius should be 10px");
}

void testInteractionTokensHaveExpectedTransparency() {
    const ThemeTokens light = themeTokens(ThemeMode::Light);
    const ThemeTokens dark = themeTokens(ThemeMode::Dark);

    require(light.rubberBandFill.a > 0.0f && light.rubberBandFill.a < 0.30f, "light rubber band fill should be translucent");
    require(dark.rubberBandFill.a > 0.0f && dark.rubberBandFill.a < 0.30f, "dark rubber band fill should be translucent");
    require(light.sidebarIconPlate.a < 1.0f, "light sidebar icon plate should be subtle");
    require(dark.sidebarIconPlate.a <= 1.0f, "dark sidebar icon plate should not exceed full opacity");
    require(light.navigationDisabled.a == 1.0f, "disabled navigation should remain opaque for readability");
    require(dark.navigationDisabled.a == 1.0f, "disabled navigation should remain opaque for readability");
}

void testAlphaHelperPreservesRgb() {
    const D2D1_COLOR_F color = D2D1::ColorF(0.1f, 0.2f, 0.3f, 1.0f);
    const D2D1_COLOR_F faded = withAlpha(color, 0.42f);

    require(faded.r == color.r, "withAlpha should preserve red");
    require(faded.g == color.g, "withAlpha should preserve green");
    require(faded.b == color.b, "withAlpha should preserve blue");
    require(faded.a == 0.42f, "withAlpha should replace alpha");
}

} // namespace

int main() {
    testThemeTokensExposeDistinctLightAndDarkSurfaces();
    testAccentStaysBlueAcrossThemes();
    testRadiiMatchDesignBaseline();
    testInteractionTokensHaveExpectedTransparency();
    testAlphaHelperPreservesRgb();
    std::cout << "ThemeTokensTests passed\n";
}
