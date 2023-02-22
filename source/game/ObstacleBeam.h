#pragma once
#include "Object.h"

class ObstacleBeam : public Object
{
public:
    enum class BEAM_TYPE
    {
        HORIZONTAL_SHORT,
        VERTICAL_LONG,
    };

    ObstacleBeam(f32 posX, f32 posY, BEAM_TYPE beamType);
    ~ObstacleBeam();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override {}

    bool CanDrag() override { return true; }
    Vector2f GetPosition() override;
    void UpdatePosition(const Vector2f& newPos) override;

private:
    static AABB CalcAABB(Vector2f pos, BEAM_TYPE beamType);
    //Vector2f m_pos;
    const BEAM_TYPE m_beamType;

    class PhysicsCollisionShapeRect* m_collisionShape;

    class Sprite* m_sprite;
};