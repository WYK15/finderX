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
    tokens.rowSelectedMutedInk = color(0.92f, 0.96f, 1.0f);
    tokens.shadow = color(0.0f, 0.0f, 0.0f, 0.14f);

    tokens.navigation = color(0.20f, 0.20f, 0.20f);
    tokens.navigationDisabled = color(0.68f, 0.68f, 0.68f);
    tokens.pathInk = color(0.34f, 0.34f, 0.34f);
    tokens.pathChevron = color(0.56f, 0.56f, 0.56f);

    tokens.sidebarIconPlate = color(0.84f, 0.90f, 1.0f, 0.55f);
    tokens.sidebarIconPlateSelected = color(0.70f, 0.82f, 1.0f, 0.55f);
    tokens.sidebarIconPlateDisabled = color(0.84f, 0.85f, 0.86f, 0.50f);
    tokens.sidebarIcon = color(0.02f, 0.45f, 0.92f);
    tokens.sidebarIconSelected = color(0.02f, 0.33f, 0.70f);
    tokens.sidebarIconDisabled = color(0.62f, 0.62f, 0.62f);

    tokens.folderTab = color(0.96f, 0.72f, 0.24f);
    tokens.folderBody = color(1.0f, 0.80f, 0.32f);
    tokens.folderHighlight = color(1.0f, 0.92f, 0.58f, 0.45f);
    tokens.selectedFolderTab = color(0.82f, 0.91f, 1.0f);
    tokens.selectedFolderBody = color(0.72f, 0.86f, 1.0f);
    tokens.selectedIconSurface = color(0.96f, 0.99f, 1.0f);
    tokens.selectedIconLine = color(0.52f, 0.74f, 1.0f);

    tokens.rubberBandFill = color(0.10f, 0.45f, 1.0f, 0.14f);
    tokens.rubberBandStroke = color(0.14f, 0.44f, 0.90f, 0.65f);
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
    tokens.rowSelectedMutedInk = color(0.92f, 0.96f, 1.0f);
    tokens.shadow = color(0.0f, 0.0f, 0.0f, 0.24f);

    tokens.navigation = color(0.88f, 0.92f, 0.98f);
    tokens.navigationDisabled = color(0.35f, 0.40f, 0.50f);
    tokens.pathInk = color(0.72f, 0.78f, 0.88f);
    tokens.pathChevron = color(0.42f, 0.48f, 0.58f);

    tokens.sidebarIconPlate = color(0.11f, 0.15f, 0.23f, 0.95f);
    tokens.sidebarIconPlateSelected = color(0.12f, 0.30f, 0.62f, 0.90f);
    tokens.sidebarIconPlateDisabled = color(0.14f, 0.16f, 0.21f, 0.70f);
    tokens.sidebarIcon = color(0.25f, 0.58f, 1.0f);
    tokens.sidebarIconSelected = color(0.55f, 0.72f, 1.0f);
    tokens.sidebarIconDisabled = color(0.35f, 0.40f, 0.50f);

    tokens.folderTab = tokens.accentFaint;
    tokens.folderBody = tokens.accentDeep;
    tokens.folderHighlight = withAlpha(tokens.accentFaint, 0.30f);
    tokens.selectedFolderTab = color(0.82f, 0.91f, 1.0f);
    tokens.selectedFolderBody = color(0.72f, 0.86f, 1.0f);
    tokens.selectedIconSurface = color(0.96f, 0.99f, 1.0f);
    tokens.selectedIconLine = color(0.52f, 0.74f, 1.0f);

    tokens.rubberBandFill = color(0.20f, 0.55f, 1.0f, 0.18f);
    tokens.rubberBandStroke = color(0.44f, 0.72f, 1.0f, 0.75f);
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
