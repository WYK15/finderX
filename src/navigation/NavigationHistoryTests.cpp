#include "navigation/NavigationHistory.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace finderx;

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void runTests() {
    {
        NavigationHistory history;
        history.setInitialPath(L"C:\\Users\\Example");

        require(history.currentPath() == L"C:\\Users\\Example", "initial path should become current path");
        require(!history.canGoBack(), "initial path should not allow back navigation");
        require(!history.canGoForward(), "initial path should not allow forward navigation");
        require(history.backTarget() == L"C:\\Users\\Example", "empty back target should be current path");
        require(history.forwardTarget() == L"C:\\Users\\Example", "empty forward target should be current path");
    }

    {
        NavigationHistory history;
        history.setInitialPath(L"C:\\Root");
        history.navigateTo(L"C:\\Root\\Documents");
        history.navigateTo(L"C:\\Root\\Pictures");

        require(history.currentPath() == L"C:\\Root\\Pictures", "navigation should update current path");
        require(history.canGoBack(), "navigation should allow back");
        require(history.backTarget() == L"C:\\Root\\Documents", "back target should be previous path");
        require(history.goBack() == L"C:\\Root\\Documents", "goBack should return previous path");
        require(history.currentPath() == L"C:\\Root\\Documents", "goBack should update current path");
        require(history.canGoForward(), "goBack should allow forward");
        require(history.forwardTarget() == L"C:\\Root\\Pictures", "forward target should be next path");
        require(history.goForward() == L"C:\\Root\\Pictures", "goForward should return next path");
        require(history.currentPath() == L"C:\\Root\\Pictures", "goForward should update current path");
    }

    {
        NavigationHistory history;
        history.setInitialPath(L"C:\\Root");

        require(history.goBack() == L"C:\\Root", "goBack at boundary should return current path");
        require(history.currentPath() == L"C:\\Root", "goBack at boundary should not change current path");
        require(history.goForward() == L"C:\\Root", "goForward at boundary should return current path");

        history.navigateTo(L"C:\\Root\\Documents");
        require(history.goBack() == L"C:\\Root", "goBack should return to initial path");
        require(history.goBack() == L"C:\\Root", "second goBack should stay at boundary");
        require(history.goForward() == L"C:\\Root\\Documents", "goForward should return to next path");
        require(history.goForward() == L"C:\\Root\\Documents", "second goForward should stay at boundary");
    }

    {
        NavigationHistory history;
        history.setInitialPath(L"C:\\Root");
        history.navigateTo(L"C:\\Root\\Documents");
        history.navigateTo(L"C:\\Root\\Pictures");
        history.goBack();
        history.navigateTo(L"C:\\Root\\Music");

        require(history.currentPath() == L"C:\\Root\\Music", "new navigation should update current path");
        require(!history.canGoForward(), "new navigation should clear forward stack");
        require(history.forwardTarget() == L"C:\\Root\\Music", "cleared forward target should be current path");
    }

    {
        NavigationHistory history;
        history.setInitialPath(L"C:\\Root");
        history.navigateTo(L"C:\\Root\\Documents");
        history.navigateTo(L"C:\\Root\\Documents");

        require(history.currentPath() == L"C:\\Root\\Documents", "same-path navigation should keep current path");
        require(history.backTarget() == L"C:\\Root", "same-path navigation should not duplicate current path");
        require(history.goBack() == L"C:\\Root", "single back should return to initial path");
        require(!history.canGoBack(), "same-path navigation should not add extra back entry");
    }
}

int main() {
    try {
        runTests();
        std::cout << "NavigationHistoryTests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
}
