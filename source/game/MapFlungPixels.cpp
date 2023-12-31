#include "MapFlungPixels.h"
#include "Map.h"

FlungPixel::FlungPixel(Map* map, Vector2f pos, Vector2f velocity, MAP_PIXEL_TYPE type, u8 seed, f32 gravity, u8 spawnChance, u32 lifetime): m_velocity(velocity), m_gravity(gravity), m_materialType(type), m_materialSeed(seed), m_spawnChance(spawnChance), m_lifetime(lifetime)
{
    m_pos = pos.Clamp(Vector2f::Zero(), Vector2f((f32)map->GetPixelWidth() - 1.0f, (f32)map->GetPixelHeight() - 1.0f));
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
    // check for pixel's EOL
    m_lifetime--;
    if (m_lifetime == 0) [[unlikely]]
        return false;

    u32 flungColor = ((u8)m_materialType << 24) | (m_materialSeed << 16);
    s32 prevPosXi = (s32)(m_pos.x + 0.5f);
    s32 prevPosYi = (s32)(m_pos.y + 0.5f);

    // calculate new position
    m_velocity.y += m_gravity;
    m_velocity += map->CalculateGravityInfluences(m_pos, 0.05f);
    m_pos += m_velocity;
    m_pos = m_pos.Clamp(Vector2f::Zero(), Vector2f((f32)map->GetPixelWidth() - 1.0f, (f32)map->GetPixelHeight() - 1.0f));
    s32 newPosXi = (s32)(m_pos.x + 0.5f);
    s32 newPosYi = (s32)(m_pos.y + 0.5f);

    // check if pixel has collided with something
    if (map->GetPixelNoBoundsCheck(newPosXi, newPosYi).IsCollideWithObjects()) [[unlikely]]
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
    for (const auto& flungPixel : m_flungPixels)
    {
        flungPixel->RemovePixelColor(this);
    }

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
