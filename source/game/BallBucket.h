#pragma once
#include "Object.h"

class BallBucket : public Object
{
public:
    BallBucket(f32 posX, f32 posY);
    ~BallBucket();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override {}

    bool CanDrag() override { return false; }
    Vector2f GetPosition() override;
    void UpdatePosition(const Vector2f& newPos) override;

private:
    static AABB CalcAABB(Vector2f pos);
    std::vector<class PhysicsCollisionShapeRect*> m_collisionShapes;
};