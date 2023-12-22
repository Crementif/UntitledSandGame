#include "MapFlungPixels.h"
#include "Map.h"

FlungPixel::FlungPixel(Map* map, Vector2f pos, Vector2f velocity, MAP_PIXEL_TYPE type, u8 seed, f32 gravity, u8 spawnChance, u32 lifetime) : m_pos(pos), m_velocity(velocity), m_gravity(gravity), m_materialType(type), m_materialSeed(seed), m_spawnChance(spawnChance), m_lifetime(lifetime)
{
    map->m_flungPixels.emplace_back(this);
}

void FlungPixel::RemovePixelColor(Map* map)
{
    s32 x = (s32)(m_pos.x + 0.5f);
    s32 y = (s32)(m_pos.y + 0.5f);
    if (map->IsPixelOOB(x, y)) {
        CriticalErrorHandler("FlungPixel::RemovePixelColor - pixel out of bounds");
    }
    map->SetPixelColor(x, y, map->GetPixelNoBoundsCheck(x, y).CalculatePixelColor());
}

bool FlungPixel::Update(Map* map)
{
    s32 prevPosXi = (s32)(m_pos.x + 0.5f);
    s32 prevPosYi = (s32)(m_pos.y + 0.5f);
    Vector2f newPos = m_pos + m_velocity;
    s32 newPosXi = (s32)(newPos.x + 0.5f);
    s32 newPosYi = (s32)(newPos.y + 0.5f);

    m_velocity.y += m_gravity;
    m_pos = newPos;

    if (m_lifetime != std::numeric_limits<u32>::max())
        m_lifetime--;
    if (m_lifetime == 0)
        return false;

    u32 flungColor = ((u8)m_materialType << 24) | (m_materialSeed << 16);

    if(prevPosXi == newPosXi && prevPosYi == newPosYi)
    {
        // even if not moved, still redraw the pixel
        map->SetPixelColor(newPosXi, newPosYi, flungColor);
        return true;
    }

    if(map->GetPixel(newPosXi, newPosYi).IsCollideWithObjects())
    {
        if ((map->GetRNGNumber()&0x7) < m_spawnChance)
            map->SpawnMaterialPixel(m_materialType, m_materialSeed, prevPosXi, prevPosYi);
        return false;
    }

    map->SetPixelColor(newPosXi, newPosYi, flungColor);

    return true;
}

void Map::SimulateFlungPixels()
{
    // first we erase the pixels
    for(auto& flungPixel : m_flungPixels)
        flungPixel->RemovePixelColor(this);
    // update and redraw
    auto it = m_flungPixels.begin();
    while(it != m_flungPixels.end())
    {
        if(!(*it)->Update(this))
        {
            it = m_flungPixels.erase(it);
            continue;
        }
        it++;
    }
}
