#pragma once
#include "../common/types.h"
#include "Map.h"

class FlungPixel
{
public:
    FlungPixel(class Map* map, Vector2f pos, Vector2f velocity, MAP_PIXEL_TYPE type, u8 seed);

    void RemovePixelColor(class Map* map);
    bool Update(class Map* map);
private:
    Vector2f m_pos;
    Vector2f m_velocity;
    MAP_PIXEL_TYPE m_materialType;
    u8 m_materialSeed;
};
