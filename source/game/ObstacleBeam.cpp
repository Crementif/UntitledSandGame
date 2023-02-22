#include "ObstacleBeam.h"
//#include "PachinkoBall.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"

Sprite* sSpriteStaticBeamHorizontalShort = nullptr;
Sprite* sSpriteStaticBeamVerticalLong = nullptr;

ObstacleBeam::ObstacleBeam(f32 posX, f32 posY, BEAM_TYPE beamType) : Object(CalcAABB({posX, posY}, beamType), false, DRAW_LAYER_0), m_beamType(beamType)
{
    if (!sSpriteStaticBeamHorizontalShort)
    {
        sSpriteStaticBeamHorizontalShort = new Sprite("tex/static_beam_0.tga", false);
        sSpriteStaticBeamVerticalLong = new Sprite("tex/static_beam_1.tga", false);
    }
    m_collisionShape = new PhysicsCollisionShapeRect(&gPhysicsMgr, m_aabb);
    // set sprite
    switch(m_beamType)
    {
        case BEAM_TYPE::HORIZONTAL_SHORT:
            m_sprite = sSpriteStaticBeamHorizontalShort;
            break;
        case BEAM_TYPE::VERTICAL_LONG:
            m_sprite = sSpriteStaticBeamVerticalLong;
            break;
    }
}

ObstacleBeam::~ObstacleBeam()
{
    delete m_collisionShape;
}

AABB ObstacleBeam::CalcAABB(Vector2f pos, BEAM_TYPE beamType)
{
    if(beamType == BEAM_TYPE::VERTICAL_LONG)
        return AABB(pos.x - 13.0, pos.y - 100.0, 26.0, 200.0);
    return AABB(pos.x - 50.0, pos.y - 13.0, 100.0, 26.0);
}

void ObstacleBeam::Draw(u32 layerIndex)
{
    Render::RenderSprite(m_sprite, m_aabb.pos.x, m_aabb.pos.y);
}

Vector2f ObstacleBeam::GetPosition()
{
    return m_aabb.GetCenter();
}

void ObstacleBeam::UpdatePosition(const Vector2f& newPos)
{
    m_aabb = CalcAABB(newPos, m_beamType);
    m_collisionShape->UpdateAABB(m_aabb);
}
