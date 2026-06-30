#pragma once

#include <d2d1helper.h>

namespace finderx {

constexpr float kDefaultDpi = 96.0f;

struct DpiScale {
    float x = kDefaultDpi;
    float y = kDefaultDpi;
};

inline float pixelsToDips(float pixels, float dpi) {
    return dpi > 0.0f ? pixels * kDefaultDpi / dpi : pixels;
}

inline D2D1_POINT_2F pixelsToDips(int x, int y, DpiScale dpi) {
    return D2D1::Point2F(
        pixelsToDips(static_cast<float>(x), dpi.x),
        pixelsToDips(static_cast<float>(y), dpi.y));
}

inline D2D1_SIZE_F pixelsToDips(unsigned int width, unsigned int height, DpiScale dpi) {
    return D2D1::SizeF(
        pixelsToDips(static_cast<float>(width), dpi.x),
        pixelsToDips(static_cast<float>(height), dpi.y));
}

} // namespace finderx
