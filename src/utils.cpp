#include "utils.h"

#include <cstdlib>

uint32_t Utils::GetNextColorU32FromPalette()
{
    static uint32_t palette[] = {
        // Material colors 500
        0xF44336FF, 0xE91E63FF, 0x9C27B0FF,
        0x673AB7FF, 0x3F51B5FF, 0x2196F3FF,
        0x03A9F4FF, 0x00BCD4FF, 0x009688FF,
        0x4CAF50FF, 0x8BC34AFF, 0xCDDC39FF,
        0xFFEB3BFF, 0xFFC107FF, 0xFF9800FF,
        0x795548FF, 0x9E9E9EFF, 0x607D8BFF,

        // Material colors A200
        0xFF5252FF, 0xFF4081FF, 0xE040FBFF,
        0x7C4DFFFF, 0x536DFEFF, 0x448AFFFF,
        0x40C4FFFF, 0x18FFFFFF, 0x64FFDAFF,
        0x69F0AEFF, 0xB2FF59FF, 0xEEFF41FF,
        0xFFFF00FF, 0xFFD740FF, 0xFFAB40FF,
        0xFF6E40FF,

        // Material colors A400
        0xFF1744FF, 0xF50057FF, 0xD500F9FF,
        0x651FFFFF, 0x3D5AFEFF, 0x2979FFFF,
        0x00B0FFFF, 0x00E5FFFF, 0x1DE9B6FF,
        0x00E676FF, 0x76FF03FF, 0xC6FF00FF,
        0xFFEA00FF, 0xFFC400FF, 0xFF9100FF,
        0xFF3D00FF,
    };

    static size_t ptr = 0;
    return palette[(ptr = (ptr + 1) % (sizeof(palette) / sizeof(uint32_t)))];
}

Color Utils::GetNextColorFromPalette()
{
    return Color::Convert(GetNextColorU32FromPalette());
}