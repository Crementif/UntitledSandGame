#pragma once
#include "Object.h"

class ObstacleCircle : public Object
{
public:
    enum class CIRCLE_TYPE
    {
        TYPE_BASIC,
    };

    ObstacleCircle(f32 posX, f32 posY, CIRCLE_TYPE type);
    ~ObstacleCircle();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override {}

    bool CanDrag() override { return true; }
    Vector2f GetPosition() override;
    void UpdatePosition(const Vector2f& newPos) override;

private:
    static AABB CalcAABB(Vector2f pos, CIRCLE_TYPE type);
    const CIRCLE_TYPE m_type;

    class PhysicsCollisionShapeCircle* m_collisionShape;

    class Sprite* m_sprite;
};