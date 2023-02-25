#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"

class Landmine : public PhysicsObject {
    friend class PhysicsObject;

public:
    Landmine(float x, float y): PhysicsObject(AABB({x, y}, {(f32)36/MAP_PIXEL_ZOOM, (f32)28/MAP_PIXEL_ZOOM}), DRAW_LAYER_0) {
    }

    static Sprite* s_landmineSprite;
private:
    u32 m_animationIdx = 1;
    void Draw(u32 layerIndex) override {
        Render::RenderSpritePortion(s_landmineSprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, (s32)(m_animationIdx/10)*36, 0, 36, 28);
    };
    void Update(float timestep) override {
        m_animationIdx++;
        if (m_animationIdx >= 100)
            m_animationIdx = 1;

        this->AddVelocity(0.0f, 1.6f);

        this->SimulatePhysics();
    };
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };
};