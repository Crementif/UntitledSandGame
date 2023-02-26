#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"
#include "GameSceneIngame.h"
#include "Particle.h"

class Landmine : public PhysicsObject {
    friend class PhysicsObject;

public:
    Landmine(GameScene* parent, u32 owner, float x, float y);

    u32 GetOwner() { return m_owner; }

    static Sprite* s_landmineSprite;
private:
    u32 m_owner = 0;

    u32 m_animationIdx = 1;
    void Draw(u32 layerIndex) override;;
    void Update(float timestep) override;
    Vector2f GetPosition() override;;
};