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

// format: RRGGBBAA
u32 _GetColorFromPixelType(PixelType& pixelType)
{
    auto matType = pixelType.GetPixelType();
    switch(matType)
    {
        case MAP_PIXEL_TYPE::AIR:
            return 0;
        case MAP_PIXEL_TYPE::SAND:
        {
            // hacky multicolor test
            u8 rd = rand()%2;
            if(rd == 0)
                return 0xE9B356FF;
            //if(rd == 1)
            //    return 0xE8B666FF;
            return 0xEEC785FF;
        }
        case MAP_PIXEL_TYPE::SOIL:
        {
            u8 rd = rand()%2;
            if(rd == 0)
                return 0x5F3300FF;
            return 0x512B00FF;
        }
        case MAP_PIXEL_TYPE::GRASS:
        {
            u8 rd = rand()%2;
            if(rd == 0)
                return 0x2F861FFF;
            return 0x3E932BFF;
        }
        case MAP_PIXEL_TYPE::LAVA:
        {
            u8 rd = rand()%3;
            if(rd == 0)
                return 0xFF0000FF;
            if(rd == 1)
                return 0xFF2B00FF;
            return 0xFF6A00FF;
        }
        case MAP_PIXEL_TYPE::ROCK:
        {
            u8 rd = rand()%9;
            if(rd <= 3)
                return 0x515152FF;
            else if(rd <= 7)
                return 0x484849FF;
            return 0x414142FF;
        }
        case MAP_PIXEL_TYPE::SMOKE:
        {
            u8 rd = rand()%2;
            if(rd == 0)
                return 0x30303080;
            return 0x50505080;
        }
        case MAP_PIXEL_TYPE::_COUNT:
            break;
    }
    return 0x12345678;
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