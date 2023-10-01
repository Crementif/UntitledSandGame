#include "MapFlungPixels.h"
#include "Map.h"

FlungPixel::FlungPixel(Map* map, Vector2f pos, Vector2f velocity, MAP_PIXEL_TYPE type, u8 seed) : m_pos(pos), m_velocity(velocity), m_materialType(type), m_materialSeed(seed)
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

    m_velocity.y += 0.028f;
    m_pos = newPos;

    u32 flungColor = ((u8)m_materialType << 24) | (m_materialSeed << 16);

    if(prevPosXi == newPosXi && prevPosYi == newPosYi)
    {
        // even if not moved, still redraw the pixel
        map->SetPixelColor(newPosXi, newPosYi, flungColor);
        return true;
    }

    if(map->GetPixel(newPosXi, newPosYi).IsCollideWithObjects())
    {
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
