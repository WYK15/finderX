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

void testTabCloseHitTargetDoesNotActivateTab() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.tabTitles = {L"First", L"Second"};

    const ChromeHitResult closeHit = chrome.hitTest(313.0f, 20.0f, rects, state);
    require(closeHit.kind == ChromeHitKind::CloseTab,
            "tab close target should hit close tab action");
    require(closeHit.tabIndex == 0,
            "tab close target should report first tab index");

    const ChromeHitResult bodyHit = chrome.hitTest(220.0f, 20.0f, rects, state);
    require(bodyHit.kind == ChromeHitKind::Tab,
            "tab body should still activate tab");
    require(bodyHit.tabIndex == 0,
            "tab body should report first tab index");
}

void testPathbarSegmentsReturnNavigationTargets() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.pathText = L"D:\\alpha\\beta";

    const ChromeHitResult driveHit = chrome.hitTest(196.0f, 684.0f, rects, state);
    require(driveHit.kind == ChromeHitKind::PathSegment,
            "drive path segment should be hittable");
    require(driveHit.path == L"D:\\",
            "drive path segment should navigate to drive root");

    const ChromeHitResult alphaHit = chrome.hitTest(244.0f, 684.0f, rects, state);
    require(alphaHit.kind == ChromeHitKind::PathSegment,
            "middle path segment should be hittable");
    require(alphaHit.path == L"D:\\alpha",
            "middle path segment should navigate to accumulated directory");

    const ChromeHitResult betaHit = chrome.hitTest(288.0f, 684.0f, rects, state);
    require(betaHit.kind == ChromeHitKind::PathSegment,
            "last path segment should be hittable");
    require(betaHit.path == L"D:\\alpha\\beta",
            "last path segment should navigate to full path");

    require(chrome.hitTest(222.0f, 684.0f, rects, state).kind == ChromeHitKind::None,
            "path chevron should not navigate");
}

void testPathbarBlankAreaStartsAddressEditing() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.pathText = L"D:\\alpha\\beta";

    const ChromeHitResult blankHit = chrome.hitTest(930.0f, 684.0f, rects, state);
    require(blankHit.kind == ChromeHitKind::AddressField,
            "pathbar blank area should start address editing");

    const ChromeHitResult segmentHit = chrome.hitTest(244.0f, 684.0f, rects, state);
    require(segmentHit.kind == ChromeHitKind::PathSegment,
            "path segment should keep precedence over address editing");
}

void testAddressEditingStillHitsAddressField() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);
    ChromeState state;
    state.pathText = L"D:\\alpha";
    state.addressEditing = true;
    state.addressText = L"D:\\alpha\\typed";

    const ChromeHitResult hit = chrome.hitTest(244.0f, 684.0f, rects, state);
    require(hit.kind == ChromeHitKind::AddressField,
            "address editing should make the whole pathbar an address field");
}

void testPathbarIgnoresStatusTextAndVirtualLocations() {
    FinderChrome chrome;
    const LayoutRects rects = chrome.layout(1000.0f, 700.0f);

    ChromeState statusState;
    statusState.pathText = L"D:\\alpha";
    statusState.statusText = L"Cannot refresh folder";
    require(chrome.hitTest(196.0f, 684.0f, rects, statusState).kind == ChromeHitKind::None,
            "status text should disable path segment hits");

    ChromeState thisPcState;
    thisPcState.pathText = L"This PC";
    require(chrome.hitTest(196.0f, 684.0f, rects, thisPcState).kind == ChromeHitKind::None,
            "virtual locations should not expose path segment hits");
}

} // namespace

int main() {
    testToolbarSortAndSettingsHitTargetsSitBeforeSearch();
    testHeaderColumnsHitUsingDrawnGeometry();
    testTabCloseHitTargetDoesNotActivateTab();
    testPathbarSegmentsReturnNavigationTargets();
    testPathbarBlankAreaStartsAddressEditing();
    testAddressEditingStillHitsAddressField();
    testPathbarIgnoresStatusTextAndVirtualLocations();
    std::cout << "FinderChromeTests passed\n";
}
