#include "ui/DpiScaling.h"

#include <cmath>
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

bool nearlyEqual(float lhs, float rhs) {
    return std::fabs(lhs - rhs) < 0.001f;
}

} // namespace

int main() {
    require(nearlyEqual(pixelsToDips(150.0f, 144.0f), 100.0f), "144 DPI should convert 150 px to 100 DIP");
    require(nearlyEqual(pixelsToDips(150.0f, 96.0f), 150.0f), "96 DPI should leave pixels unchanged");
    require(nearlyEqual(pixelsToDips(150.0f, 0.0f), 150.0f), "invalid DPI should leave pixels unchanged");

    const DpiScale scale{144.0f, 192.0f};
    const D2D1_POINT_2F point = pixelsToDips(150, 200, scale);
    require(nearlyEqual(point.x, 100.0f), "x coordinate should use x DPI");
    require(nearlyEqual(point.y, 100.0f), "y coordinate should use y DPI");

    std::cout << "DpiScalingTests passed\n";
    return 0;
}
