#include "Landmine.h"
#include "Player.h"

Landmine::Landmine(GameScene *parent, u32 owner, float x, float y) : PhysicsObject(parent, AABB({x, y}, {(f32)36/MAP_PIXEL_ZOOM, (f32)28/MAP_PIXEL_ZOOM}), DRAW_LAYER_0), m_owner(owner) {
}

void Landmine::Update(float timestep) {
    m_animationIdx++;
    if (m_animationIdx >= 100)
        m_animationIdx = 1;

    this->AddVelocity(0.0f, 1.6f);
    this->SimulatePhysics();

    for (auto& playerIt : m_parent->GetPlayers()) {
        if (playerIt.first == this->m_owner && playerIt.second->GetPosition().Distance(this->GetPosition()) > 80.0f) {
            new ExplosiveParticle(m_parent, std::make_unique<Sprite>("/tex/ball.tga"), Vector2f(m_aabb.pos), 8, 2.0f, 80, 20.0f, 20.0f);
            playerIt.second->TakeDamage();
            m_parent->QueueUnregisterObject(this);
        }
    }
}

Vector2f Landmine::GetPosition() {
    return m_aabb.GetCenter();
}

void Landmine::Draw(u32 layerIndex) {
    Render::RenderSpritePortion(s_landmineSprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, (s32)(m_animationIdx/10)*36, 0, 36, 28);
}