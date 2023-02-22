#include "BallBucket.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"

Sprite* sSpriteBucket_bg = nullptr;
Sprite* sSpriteBucket_front = nullptr;

BallBucket::BallBucket(f32 posX, f32 posY) : Object(CalcAABB({posX, posY}), false, DRAW_LAYER_0 | DRAW_LAYER_2)
{
    if (!sSpriteBucket_bg)
    {
        sSpriteBucket_bg = new Sprite("tex/bucket_layer_bg.tga", true);
        sSpriteBucket_front = new Sprite("tex/bucket_layer_front.tga", true);
    }
    Vector2f pos = m_aabb.pos;
    m_collisionShapes.emplace_back(new PhysicsCollisionShapeRect(&gPhysicsMgr, AABB(pos.x, pos.y, 27.0f, 256.0f)));
    m_collisionShapes.emplace_back(new PhysicsCollisionShapeRect(&gPhysicsMgr, AABB(pos.x + 256.0f - 27.0f, pos.y, 27.0f, 256.0f)));
    m_collisionShapes.emplace_back(new PhysicsCollisionShapeRect(&gPhysicsMgr, AABB(pos.x + 0.0f, pos.y + 256.0f - 10.0f, 256.0f, 10.0f)));
}

BallBucket::~BallBucket()
{
    for(auto& it : m_collisionShapes)
        delete it;
    m_collisionShapes.clear();
}

AABB BallBucket::CalcAABB(Vector2f pos)
{
    return AABB(pos.x - 128.0, pos.y - 128.0, 256.0, 256.0);
}

void BallBucket::Draw(u32 layerIndex)
{
    if(layerIndex == 0)
        Render::RenderSprite(sSpriteBucket_bg, m_aabb.pos.x, m_aabb.pos.y);
    else if(layerIndex == 2)
        Render::RenderSprite(sSpriteBucket_front, m_aabb.pos.x, m_aabb.pos.y);
}

Vector2f BallBucket::GetPosition()
{
    return m_aabb.GetCenter();
}

void BallBucket::UpdatePosition(const Vector2f& newPos)
{
    //m_aabb = CalcAABB(newPos);
    //m_collisionShape->UpdateAABB(m_aabb);
}
