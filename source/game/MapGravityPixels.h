#pragma once
#include "../common/types.h"
#include "Map.h"

class GravityPixel {
public:
    GravityPixel(class Map* map, Vector2f pos, f32 strength, u32 lifetime = std::numeric_limits<u32>::max());
    Vector2f CalculateInfluence(Vector2f influencedPos, f32 influencedWeight) const;
    bool Update(class Map* map);

private:
    Vector2f m_pos;
    f32 m_strength;
    u32 m_lifetime;
};
