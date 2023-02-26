#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"
#include "GameSceneIngame.h"
#include "Particle.h"

class Landmine : public PhysicsObject {
    friend class PhysicsObject;

public:
    Landmine(u32 owner, float x, float y): PhysicsObject(AABB({x, y}, {(f32)36/MAP_PIXEL_ZOOM, (f32)28/MAP_PIXEL_ZOOM}), DRAW_LAYER_0), m_owner(owner) {
    }

    u32 m_owner = 0;

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

        GameScene* scene = GameScene::GetCurrent();
        for (auto& playerIt : scene->GetPlayers()) {
            if (playerIt.first != this->m_owner && playerIt.second->GetPosition().Distance(this->GetPosition()) < 80.0f) {
                new Particle(std::make_unique<Sprite>("/tex/ball.tga"), Vector2f(m_aabb.pos), 8, 10.0f, 80, 20.0f);
                playerIt.second->TakeDamage();
                Object::QueueForDeletion(this);
            }
        }
    };
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };
};