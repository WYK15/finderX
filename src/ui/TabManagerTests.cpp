#include "ui/TabManager.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace finderx::ui;

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    {
        TabManager tabs;

        require(tabs.count() == 0, "default constructed tab manager should start empty");
        require(!tabs.hasActive(), "default constructed tab manager should not have an active tab");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\Users\\Example");

        require(tabs.count() == 1, "initialize should create one tab");
        require(tabs.hasActive(), "initialize should create an active tab");
        require(tabs.activeIndex() == 0, "initial tab should be active");
        require(tabs.active().path == L"C:\\Users\\Example", "initial tab path should be stored");
        require(tabs.active().title == L"Example", "initial tab title should use final path component");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\Users\\Example");
        tabs.active().searchText = L"doc";

        const std::size_t created = tabs.createTab(L"C:\\Users\\Example\\Downloads");
        require(created == 1, "created tab index should be returned");
        require(tabs.count() == 2, "createTab should append a tab");
        require(tabs.activeIndex() == 1, "new tab should become active");
        require(tabs.active().path == L"C:\\Users\\Example\\Downloads", "new tab path should be stored");
        require(tabs.active().title == L"Downloads", "new tab title should use final path component");
        require(tabs.active().searchText.empty(), "new tab search should start empty");

        tabs.activate(0);
        require(tabs.activeIndex() == 0, "activate should switch active index");
        require(tabs.active().searchText == L"doc", "search text should be isolated per tab");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\Users\\Example");
        tabs.createTab(L"C:\\Users\\Example\\Desktop");
        tabs.activate(99);
        require(tabs.activeIndex() == 1, "invalid activation should be ignored");
    }

    {
        TabManager tabs;
        tabs.initialize(L"C:\\");
        require(tabs.active().title == L"C:\\", "root-like path should fall back to full path");
    }

    std::cout << "TabManagerTests passed\n";
    return 0;
}
