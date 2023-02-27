#include "MapFlungPixels.h"

u32 _GetColorFromPixelType(PixelType& pixelType);

FlungPixel::FlungPixel(class Map* map, Vector2f pos, Vector2f velocity, MAP_PIXEL_TYPE type) : m_pos(pos), m_velocity(velocity), m_materialType(type)
{
    map->m_flungPixels.emplace_back(this);
}

void FlungPixel::RemovePixelColor(class Map* map)
{
    s32 x = (s32)(m_pos.x + 0.5f);
    s32 y = (s32)(m_pos.y + 0.5f);
    map->SetPixelColor(x, y, _GetColorFromPixelType(map->GetPixel(x, y)));
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

    // hacky workaround to get color, eventually decouple _GetColorFromPixelType from PixelType
    PixelType pt;
    pt.SetPixel(m_materialType);
    u32 pixelColor = _GetColorFromPixelType(pt);

    if(prevPosXi == newPosXi && prevPosYi == newPosYi)
    {
        // even if not moved, still redraw the pixel
        map->SetPixelColor(newPosXi, newPosYi, pixelColor);
        return true;
    }


    if(map->GetPixel(newPosXi, newPosYi).IsCollideWithObjects())
    {
        return false;
    }

    map->SetPixelColor(newPosXi, newPosYi, pixelColor);

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
