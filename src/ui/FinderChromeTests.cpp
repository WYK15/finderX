#include "ui/FinderChrome.h"

#include <cstdlib>
#include <iostream>

using namespace finderx;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        std::exit(1);
    }
}

void testToolbarSortAndSettingsHitTargetsSitBeforeSearch() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;

    require(chrome.hitTest(735.0f, 63.0f, rects, state).kind == ChromeHitKind::SortMenu,
            "sort toolbar button should hit before search");
    require(chrome.hitTest(775.0f, 63.0f, rects, state).kind == ChromeHitKind::Settings,
            "settings toolbar button should hit before search");
    require(chrome.hitTest(900.0f, 63.0f, rects, state).kind == ChromeHitKind::SearchField,
            "search field should remain hittable");
}

void testHeaderColumnsHitUsingDrawnGeometry() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;

    require(chrome.hitTest(250.0f, 96.0f, rects, state).kind == ChromeHitKind::HeaderName,
            "name header should hit in name column");
    require(chrome.hitTest(660.0f, 96.0f, rects, state).kind == ChromeHitKind::HeaderModified,
            "modified header should hit in modified column");
    require(chrome.hitTest(810.0f, 96.0f, rects, state).kind == ChromeHitKind::HeaderSize,
            "size header should hit in size column");
    require(chrome.hitTest(900.0f, 96.0f, rects, state).kind == ChromeHitKind::HeaderKind,
            "kind header should hit in kind column");
}

} // namespace

int main() {
    testToolbarSortAndSettingsHitTargetsSitBeforeSearch();
    testHeaderColumnsHitUsingDrawnGeometry();
    std::cout << "FinderChromeTests passed\n";
}
