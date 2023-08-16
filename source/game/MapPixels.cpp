#include "Map.h"
#include "MapPixels.h"

MAP_PIXEL_TYPE _GetPixelTypeFromTGAColor(u32 c)
{
    if(c == 0x7F3300)
        return MAP_PIXEL_TYPE::SOIL;
    else if(c == 0xFFD800)
        return MAP_PIXEL_TYPE::SAND;
    else if(c == 0x007F0E)
        return MAP_PIXEL_TYPE::GRASS;
    else if(c == 0xFF0000)
        return MAP_PIXEL_TYPE::LAVA;
    else if(c == 0x808080)
        return MAP_PIXEL_TYPE::ROCK;

    return MAP_PIXEL_TYPE::AIR;
}

// format: 0xRRGGBBAA
u32 _GetColorFromPixelType(PixelType& pixelType)
{
    auto matType = pixelType.GetPixelType();
    u32 type = (u8)matType;

    u32 seed = 0;
    switch(matType)
    {
        case MAP_PIXEL_TYPE::AIR:
            break;
        case MAP_PIXEL_TYPE::SAND:
        {
            seed = rand()%2;
            break;
        }
        case MAP_PIXEL_TYPE::SOIL:
        {
            seed = rand()%2;
            break;
        }
        case MAP_PIXEL_TYPE::GRASS:
        {
            seed = rand()%2;
            break;
        }
        case MAP_PIXEL_TYPE::LAVA:
        {
            seed = rand()%3;
            break;
        }
        case MAP_PIXEL_TYPE::ROCK:
        {
            seed = rand()%9;
            break;
        }
        case MAP_PIXEL_TYPE::SMOKE:
        {
            seed = rand()%2;
            break;
        }
        case MAP_PIXEL_TYPE::_COUNT:
            break;
    }
    return (type << 24) | (seed << 16);
}

u32 _CalculateDimColor(u32 color, float dimFactor) {
    u8 r = (color >> 24) & 0xFF;
    u8 g = (color >> 16) & 0xFF;
    u8 b = (color >> 8) & 0xFF;
    u8 a = color & 0xFF;

    r = std::max(0, static_cast<int>(r * (1 - dimFactor)));
    g = std::max(0, static_cast<int>(g * (1 - dimFactor)));
    b = std::max(0, static_cast<int>(b * (1 - dimFactor)));

    return (r << 24) | (g << 16) | (b << 8) | a;
}