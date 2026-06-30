#include "ui/DirectoryRefreshDebouncer.h"

#include <cstdlib>
#include <iostream>

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
        DirectoryRefreshDebouncer debouncer(250);
        require(!debouncer.pending(), "new debouncer should not be pending");
        require(!debouncer.consumeIfDue(1000), "new debouncer should not refresh");

        debouncer.request(1000);
        require(debouncer.pending(), "request should mark refresh pending");
        require(!debouncer.consumeIfDue(1249), "refresh should wait until debounce delay");
        require(debouncer.consumeIfDue(1250), "refresh should fire at debounce delay");
        require(!debouncer.pending(), "consumed refresh should clear pending state");
        require(!debouncer.consumeIfDue(1500), "refresh should only fire once per request");
    }

    {
        DirectoryRefreshDebouncer debouncer(250);
        debouncer.request(1000);
        debouncer.request(1100);
        require(!debouncer.consumeIfDue(1250), "second request should extend debounce delay");
        require(debouncer.consumeIfDue(1350), "refresh should fire after latest request delay");
    }

    {
        DirectoryRefreshDebouncer debouncer(250);
        debouncer.request(1000);
        debouncer.cancel();
        require(!debouncer.pending(), "cancel should clear pending refresh");
        require(!debouncer.consumeIfDue(2000), "cancelled refresh should not fire");
    }

    std::cout << "DirectoryRefreshDebouncerTests passed\n";
    return 0;
}
