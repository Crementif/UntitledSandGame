#pragma once
#include "Object.h"

class Splitter : public Object
{
public:
    Splitter(f32 posX, f32 posY);
    ~Splitter();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    bool CanDrag() override { return true; }
    Vector2f GetPosition() override;
    void UpdatePosition(const Vector2f& newPos) override;

private:
    static AABB CalcAABB(Vector2f pos);
    AABB CalcCollisionAABB(u32 index, Vector2f pos);
    std::vector<class PhysicsCollisionShapeRect*> m_collisionShapes;
};