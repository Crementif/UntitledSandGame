#pragma once
#include "Map.h"
#include <vector>

#include <coreinit/debug.h>

template<MAP_PIXEL_TYPE TMaterial>
class ActivePixel
{
public:
    ActivePixel(s32 x, s32 y) : x((u16)x), y((u16)y)
    {

    }

    void RemoveFromWorld(Map* map)
    {
        map->GetPixel(x, y).SetPixel(MAP_PIXEL_TYPE::AIR);
    }

    void IntegrateIntoWorld(Map* map)
    {
        map->GetPixel(x, y).SetPixel(TMaterial);
    }

    void ChangeParticleXY(Map* map, s32 x, s32 y)
    {
        RemoveFromWorld(map);
        this->x = x;
        this->y = y;
        IntegrateIntoWorld(map);
    }

    uint16_t x;
    uint16_t y;
};

class ActivePixelSand : public ActivePixel<MAP_PIXEL_TYPE::SAND>
{
public:
    ActivePixelSand(s32 x, s32 y) : ActivePixel<MAP_PIXEL_TYPE::SAND>(x, y) {};

    inline bool SimulateStep(Map* map)
    {
        if(!map->GetPixel(x, y+1).IsSolid())
        {
            ChangeParticleXY(map, x, y+1);
            return true;
        }
        // try to move either left or right
        if(!map->GetPixel(x - 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x - 1, y+1);
            return true;
        }
        if(!map->GetPixel(x + 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x + 1, y+1);
            return true;
        }

        return true;
    }
};

class ActivePixelCollection
{
public:
    std::vector<ActivePixelSand> sandPixels;
    //std::vector<ActivePixelSand> sandPixels;

    //void SimulateStep(Map_t* map)
    //{

    //}
};