#include "MapGravityPixels.h"
#include "Map.h"

GravityPixel::GravityPixel(Map* map, Vector2f pos, f32 strength, u32 lifetime) : m_pos(pos), m_strength(strength), m_lifetime(lifetime)
{
    map->m_gravityPixels.emplace_back(this);
}

bool GravityPixel::Update(Map* map)
{
    m_lifetime--;
    if (m_lifetime == 0) [[unlikely]]
        return false;

    return true;
}


Vector2f GravityPixel::CalculateInfluence(const Vector2f influencedPos, const f32 influencedWeight) const {
    Vector2f direction = m_pos - influencedPos;
    f32 distance = direction.Length();

    f32 influenceStrength = m_strength / distance;
    Vector2f influence = direction.GetNormalized() * influenceStrength * influencedWeight;

    return influence;
}

Vector2f Map::CalculateGravityInfluences(Vector2f pos, f32 weight) const {
    Vector2f combinedInfluences = {0.0f, 0.0f};
    for (const auto& gravityPixel : m_gravityPixels)
    {
        combinedInfluences += gravityPixel->CalculateInfluence(pos, weight);
    }

    return combinedInfluences;
}

void Map::SimulateGravityPixels()
{
    // update and erase any gravity pixels that are EOL
    for (auto it = m_gravityPixels.begin(); it != m_gravityPixels.end();)
    {
        if (!(*it)->Update(this))
        {
            delete *it;
            it = m_gravityPixels.erase(it);
        }
        else
            ++it;
    }
}