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
