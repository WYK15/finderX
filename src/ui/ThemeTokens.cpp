#include "ui/ThemeTokens.h"

namespace finderx {

namespace {

D2D1_COLOR_F color(float r, float g, float b, float a = 1.0f) {
    return D2D1::ColorF(r, g, b, a);
}

ThemeTokens lightTokens() {
    ThemeTokens tokens;
    tokens.accent = color(0.00f, 0.47f, 0.95f);
    tokens.accentFaint = color(0.48f, 0.72f, 1.0f);
    tokens.accentDeep = color(0.00f, 0.34f, 0.78f);

    tokens.ink = color(0.15f, 0.15f, 0.16f);
    tokens.inkDull = color(0.36f, 0.37f, 0.39f);
    tokens.inkFaint = color(0.52f, 0.53f, 0.56f);
    tokens.inkOnAccent = color(1.0f, 1.0f, 1.0f);

    tokens.app = color(0.985f, 0.985f, 0.980f);
    tokens.appBox = color(1.0f, 1.0f, 1.0f);
    tokens.appOverlay = color(0.965f, 0.970f, 0.980f);
    tokens.appInput = color(1.0f, 1.0f, 1.0f);
    tokens.appLine = color(0.82f, 0.83f, 0.85f);
    tokens.appHover = color(0.945f, 0.950f, 0.960f);
    tokens.appSelected = color(0.84f, 0.88f, 0.95f);
    tokens.appActive = color(0.78f, 0.84f, 0.93f);

    tokens.sidebar = color(0.925f, 0.930f, 0.940f);
    tokens.sidebarBox = color(0.965f, 0.970f, 0.980f);
    tokens.sidebarLine = color(0.78f, 0.80f, 0.83f);
    tokens.sidebarSelected = color(0.80f, 0.84f, 0.90f);

    tokens.menu = color(0.980f, 0.982f, 0.988f);
    tokens.menuLine = color(0.82f, 0.84f, 0.88f);
    tokens.menuHover = tokens.accent;
    tokens.menuSelected = color(0.22f, 0.28f, 0.36f);

    tokens.statusError = color(0.72f, 0.18f, 0.16f);
    tokens.rowStripe = color(0.965f, 0.965f, 0.965f);
    tokens.rowSelected = color(0.00f, 0.42f, 0.88f);
    tokens.shadow = color(0.0f, 0.0f, 0.0f, 0.14f);
    return tokens;
}

ThemeTokens darkTokens() {
    ThemeTokens tokens;
    tokens.accent = color(0.20f, 0.55f, 1.0f);
    tokens.accentFaint = color(0.55f, 0.74f, 1.0f);
    tokens.accentDeep = color(0.10f, 0.34f, 0.78f);

    tokens.ink = color(0.90f, 0.93f, 0.98f);
    tokens.inkDull = color(0.60f, 0.66f, 0.76f);
    tokens.inkFaint = color(0.43f, 0.49f, 0.59f);
    tokens.inkOnAccent = color(1.0f, 1.0f, 1.0f);

    tokens.app = color(0.070f, 0.085f, 0.120f);
    tokens.appBox = color(0.105f, 0.130f, 0.190f);
    tokens.appOverlay = color(0.095f, 0.115f, 0.160f);
    tokens.appInput = color(0.095f, 0.115f, 0.160f);
    tokens.appLine = color(0.22f, 0.27f, 0.36f);
    tokens.appHover = color(0.13f, 0.16f, 0.22f);
    tokens.appSelected = color(0.13f, 0.22f, 0.36f);
    tokens.appActive = color(0.18f, 0.25f, 0.38f);

    tokens.sidebar = color(0.055f, 0.070f, 0.105f);
    tokens.sidebarBox = color(0.11f, 0.15f, 0.23f);
    tokens.sidebarLine = color(0.16f, 0.19f, 0.26f);
    tokens.sidebarSelected = color(0.13f, 0.22f, 0.36f);

    tokens.menu = color(0.080f, 0.100f, 0.145f);
    tokens.menuLine = color(0.16f, 0.19f, 0.26f);
    tokens.menuHover = tokens.accent;
    tokens.menuSelected = color(0.20f, 0.26f, 0.36f);

    tokens.statusError = color(0.92f, 0.35f, 0.32f);
    tokens.rowStripe = color(0.095f, 0.115f, 0.155f);
    tokens.rowSelected = color(0.12f, 0.34f, 0.74f);
    tokens.shadow = color(0.0f, 0.0f, 0.0f, 0.24f);
    return tokens;
}

} // namespace

bool isDarkTheme(ThemeMode mode) {
    return mode == ThemeMode::Dark;
}

ThemeTokens themeTokens(ThemeMode mode) {
    return isDarkTheme(mode) ? darkTokens() : lightTokens();
}

D2D1_COLOR_F withAlpha(D2D1_COLOR_F color, float alpha) {
    color.a = alpha;
    return color;
}

} // namespace finderx
