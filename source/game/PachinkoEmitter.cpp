#include "PachinkoEmitter.h"
#include "PachinkoBall.h"
#include "../framework/render.h"

Sprite* sSpritePachinkoEmitter = nullptr;

PachinkoEmitter::PachinkoEmitter(f32 posX, f32 posY) : Object(CalcAABB(), true, DRAW_LAYER_2), m_pos(posX, posY)
{
    if (!sSpritePachinkoEmitter)
        sSpritePachinkoEmitter = new Sprite("tex/emitter.tga", true);
}

PachinkoEmitter::~PachinkoEmitter()
{

}

AABB PachinkoEmitter::CalcAABB()
{
    return AABB(m_pos.x - 40.0, m_pos.y - 40.0, 80.0, 80.0);
}

void PachinkoEmitter::Draw(u32 layerIndex)
{
    Render::RenderSprite(sSpritePachinkoEmitter, m_pos.x - 40, m_pos.y - 40);
}

void PachinkoEmitter::Update(float timestep)
{
    m_timer += timestep;
    if(m_timer >= 5.00f)
    {
        // adding a tiny variance to the position helps with avoiding ball stacking, but it also makes the movement non-deterministic
        //float minisculeVariance = (float)(rand()%400) * 0.0001;
        float minisculeVariance = 0.0f;
        new PachinkoBall(m_pos.x + minisculeVariance, m_pos.y - 20.0f, PachinkoBall::BALL_TYPE::RED);
        m_timer -= 5.00f;
    }
}

Vector2f PachinkoEmitter::GetPosition()
{
    return {m_pos.x - 40, m_pos.y - 20.0f};
}