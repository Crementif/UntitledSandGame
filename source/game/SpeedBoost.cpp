#include "SpeedBoost.h"

#include "PachinkoBall.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"

SpeedBoost::SpeedBoost(f32 posX, f32 posY) : Object(CalcAABB({posX, posY}), true, DRAW_LAYER_2)
{
    if (this->sprites[0] == nullptr)
    {
        this->sprites[0] = new Sprite("tex/speed_boost_0.tga", true);
        this->sprites[1] = new Sprite("tex/speed_boost_1.tga", true);
        this->sprites[2] = new Sprite("tex/speed_boost_2.tga", true);
        this->sprites[3] = new Sprite("tex/speed_boost_3.tga", true);
    }
}

SpeedBoost::~SpeedBoost()
{
}

AABB SpeedBoost::CalcAABB(Vector2f pos)
{
    return AABB(pos.x - (80.0/2.0), pos.y - (96.0/2.0), 80.0, 96.0);
}

void SpeedBoost::Draw(u32 layerIndex)
{
    Render::RenderSprite(sprites[spriteIndex], m_aabb.pos.x-10, m_aabb.pos.y);
}

Vector2f SpeedBoost::GetPosition()
{
    return m_aabb.GetCenter();
}

void SpeedBoost::UpdatePosition(const Vector2f& newPos)
{
    m_aabb = CalcAABB(newPos);
}

void SpeedBoost::Update(float timestep)
{
    auto balls = PachinkoBall::GetAllBalls();
    for(auto& it : balls)
    {
        if(m_aabb.Contains(it->GetPosition()))
        {
            it->ApplyForce({20.0f, 4.0f});
        }
    }

    timeDeltaLeft -= timestep;
    if (timeDeltaLeft < 0.0) {
        timeDeltaLeft = 0.25;
        spriteIndex++;
        if (spriteIndex > sprites.size()-1) {
            spriteIndex = 0;
        }
    }
}