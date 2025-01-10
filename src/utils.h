#pragma once

#include <cstdint>

#define EVAL_ONCE(expression)       \
    {                               \
        static bool done = false;   \
        if (!done) {                \
            done = true;            \
            (expression);           \
        }                           \
    }

struct Color {
    float r = 0.0, g = 0.0, b = 0.0, a = 0.0;
};

constexpr uint32_t Reverse(uint32_t n)
{
    return ((n << 24) & 0xff000000) + ((n << 8)  & 0x00ff0000)
         + ((n >> 8)  & 0x0000ff00) + ((n >> 24) & 0x000000ff);
}

constexpr uint32_t Convert(Color c)
{
    return ((uint32_t(c.r * 255) << 24) & 0xff000000)
         + ((uint32_t(c.g * 255) << 16) & 0x00ff0000)
         + ((uint32_t(c.b * 255) << 8)  & 0x0000ff00)
         +  (uint32_t(c.a * 255)        & 0x000000ff);
}

constexpr Color Convert(uint32_t c)
{
    return {
         ((c >> 24) & 0x000000ff) / 255.0f,
         ((c >> 16) & 0x000000ff) / 255.0f,
         ((c >> 8)  & 0x000000ff) / 255.0f,
          (c        & 0x000000ff) / 255.0f,
    };
}

namespace Utils {

    uint32_t GetNextColorU32FromPalette();
    Color    GetNextColorFromPalette();

} // namespace Utils