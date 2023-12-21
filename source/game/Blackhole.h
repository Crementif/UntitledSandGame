#pragma once

#include "Object.h"
#include "../framework/render.h"

class Blackhole : public PhysicsObject {
    friend class PhysicsObject;

public:
    Blackhole(GameScene* parent, u32 owner, float x, float y, float volX, float volY);

    u32 GetOwner() const { return m_owner; }

    static Sprite* s_blackhole0Sprite;
    static Sprite* s_blackhole1Sprite;
    static Sprite* s_blackhole2Sprite;
private:
    u32 m_owner = 0;

    u32 m_animationIdx = 1;
    float m_velocityAngle = 0.0f;
    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;
    Vector2f GetPosition() override;
};
