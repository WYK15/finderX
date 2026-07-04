#pragma once

#include "settings/AppSettings.h"

#include <d2d1.h>

namespace finderx {

struct ThemeTokens {
    D2D1_COLOR_F accent{};
    D2D1_COLOR_F accentFaint{};
    D2D1_COLOR_F accentDeep{};

    D2D1_COLOR_F ink{};
    D2D1_COLOR_F inkDull{};
    D2D1_COLOR_F inkFaint{};
    D2D1_COLOR_F inkOnAccent{};

    D2D1_COLOR_F app{};
    D2D1_COLOR_F appBox{};
    D2D1_COLOR_F appOverlay{};
    D2D1_COLOR_F appInput{};
    D2D1_COLOR_F appLine{};
    D2D1_COLOR_F appHover{};
    D2D1_COLOR_F appSelected{};
    D2D1_COLOR_F appActive{};

    D2D1_COLOR_F sidebar{};
    D2D1_COLOR_F sidebarBox{};
    D2D1_COLOR_F sidebarLine{};
    D2D1_COLOR_F sidebarSelected{};

    D2D1_COLOR_F menu{};
    D2D1_COLOR_F menuLine{};
    D2D1_COLOR_F menuHover{};
    D2D1_COLOR_F menuSelected{};

    D2D1_COLOR_F statusError{};
    D2D1_COLOR_F rowStripe{};
    D2D1_COLOR_F rowSelected{};
    D2D1_COLOR_F rowSelectedMutedInk{};
    D2D1_COLOR_F shadow{};

    D2D1_COLOR_F navigation{};
    D2D1_COLOR_F navigationDisabled{};
    D2D1_COLOR_F pathInk{};
    D2D1_COLOR_F pathChevron{};

    D2D1_COLOR_F sidebarIconPlate{};
    D2D1_COLOR_F sidebarIconPlateSelected{};
    D2D1_COLOR_F sidebarIconPlateDisabled{};
    D2D1_COLOR_F sidebarIcon{};
    D2D1_COLOR_F sidebarIconSelected{};
    D2D1_COLOR_F sidebarIconDisabled{};

    D2D1_COLOR_F folderTab{};
    D2D1_COLOR_F folderBody{};
    D2D1_COLOR_F folderHighlight{};
    D2D1_COLOR_F selectedFolderTab{};
    D2D1_COLOR_F selectedFolderBody{};
    D2D1_COLOR_F selectedIconSurface{};
    D2D1_COLOR_F selectedIconLine{};

    D2D1_COLOR_F rubberBandFill{};
    D2D1_COLOR_F rubberBandStroke{};

    float radiusControl = 6.0f;
    float radiusPanel = 8.0f;
    float radiusWindow = 10.0f;
};

bool isDarkTheme(ThemeMode mode);
ThemeTokens themeTokens(ThemeMode mode);
D2D1_COLOR_F withAlpha(D2D1_COLOR_F color, float alpha);

} // namespace finderx
