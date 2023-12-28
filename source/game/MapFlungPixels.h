#pragma once
#include "../common/types.h"
#include "Map.h"

class FlungPixel
{
public:
    FlungPixel(class Map* map, Vector2f pos, Vector2f velocity, MAP_PIXEL_TYPE type, u8 seed, f32 gravity = 0.028f, u8 spawnChance = 0x5, u32 lifetime = std::numeric_limits<u32>::max());

    void RemovePixelColor(class Map* map);
    bool Update(class Map* map);
private:
    Vector2f m_pos;
    Vector2f m_velocity;
    f32 m_gravity;
    MAP_PIXEL_TYPE m_materialType;
    u8 m_materialSeed;
    u8 m_spawnChance;
    u32 m_lifetime;
};
