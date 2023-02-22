#include "ObstacleCircle.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"

Sprite* sSpriteStaticCircle0 = nullptr;

ObstacleCircle::ObstacleCircle(f32 posX, f32 posY, CIRCLE_TYPE type) : Object(CalcAABB({posX, posY}, type), false, DRAW_LAYER_0), m_type(type)
{
    if (!sSpriteStaticCircle0)
    {
        sSpriteStaticCircle0 = new Sprite("tex/obstacle_circle_0.tga", true);
    }
    m_collisionShape = new PhysicsCollisionShapeCircle(&gPhysicsMgr, {posX, posY}, 16.0f);
    // set sprite
    switch(m_type)
    {
        case CIRCLE_TYPE::TYPE_BASIC:
            m_sprite = sSpriteStaticCircle0;
            break;
    }
}

ObstacleCircle::~ObstacleCircle()
{
    delete m_collisionShape;
}

AABB ObstacleCircle::CalcAABB(Vector2f pos, CIRCLE_TYPE beamType)
{
    return AABB(pos.x - 16.0, pos.y - 16.0, 32.0, 32.0);
}

void ObstacleCircle::Draw(u32 layerIndex)
{
    Render::RenderSprite(m_sprite, m_aabb.pos.x, m_aabb.pos.y);
}

Vector2f ObstacleCircle::GetPosition()
{
    return m_aabb.GetCenter();
}

void ObstacleCircle::UpdatePosition(const Vector2f& newPos)
{
    m_aabb = CalcAABB(newPos, m_type);
    m_collisionShape->UpdatePosition(newPos);
}
