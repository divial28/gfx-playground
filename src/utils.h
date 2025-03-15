#pragma once

#include <imgui.h>
#include <cassert>
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

    static constexpr uint32_t Convert(Color c)
    {
        return ((uint32_t(c.r * 255) << 24) & 0xff000000)
            + ((uint32_t(c.g * 255) << 16) & 0x00ff0000)
            + ((uint32_t(c.b * 255) << 8)  & 0x0000ff00)
            +  (uint32_t(c.a * 255)        & 0x000000ff);
    }

    static constexpr Color Convert(uint32_t c)
    {
        return {
            ((c >> 24) & 0x000000ff) / 255.0f,
            ((c >> 16) & 0x000000ff) / 255.0f,
            ((c >> 8)  & 0x000000ff) / 255.0f,
            (c        & 0x000000ff) / 255.0f,
        };
    }

    // for vscode colorpicker
    static constexpr Color rgb(uint8_t r, uint8_t g, uint8_t b) // NOLINT
    {
        return { r / 255.0f, g / 255.0f, b / 255.0f, 1.0f };
    }
    static constexpr Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) // NOLINT
    {
        return { r / 255.0f, g / 255.0f, b / 255.0f, a  / 255.0f };
    }
    static constexpr Color hsl(float h, float s, float l) // NOLINT
    {
        return {};
    }
    static constexpr Color hsla(float h, float s, float l, float a) // NOLINT
    {
        return {};
    }
};

constexpr uint32_t Reverse(uint32_t n)
{
    return ((n << 24) & 0xff000000) + ((n << 8)  & 0x00ff0000)
         + ((n >> 8)  & 0x0000ff00) + ((n >> 24) & 0x000000ff);
}

inline bool operator > (const ImVec2& l, const ImVec2& r)
{
    return l.x > r.x && l.y > r.y;
}

inline bool operator < (const ImVec2& l, const ImVec2& r)
{
    return r > l;
}

namespace Utils {

    uint32_t GetNextColorU32FromPalette();
    Color    GetNextColorFromPalette();

} // namespace Utils