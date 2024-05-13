#include "Landmine.h"
#include "Player.h"

#include "../framework/audio/audio.h"

Landmine::Landmine(GameScene *parent, u32 owner, float x, float y) : PhysicsObject(parent, AABB({x, y}, {(f32)36/MAP_PIXEL_ZOOM, (f32)28/MAP_PIXEL_ZOOM}), DRAW_LAYER_0), m_owner(owner) {
}

void Landmine::Update(float timestep) {
    m_animationIdx++;
    if (m_animationIdx >= 100)
        m_animationIdx = 1;

    this->AddVelocity(0.0f, 1.6f);
    this->SimulatePhysics();

    auto triggerExplosion = [this](Player* player) {
        new ExplosiveParticle(m_parent, Vector2f(m_aabb.GetTopLeft().x-11.0f, m_aabb.GetTopLeft().y-11.0f), 8, 1.8f, 2, 20.0f, 20.0f);

        float distance = (m_parent->GetPlayer()->GetPosition().Distance(m_aabb.GetCenter())+0.00000001f)/20.0f;
        float volume = 20.0f - (distance/100.0f*20.0f);

        Audio* explosionAudio = new Audio("/sfx/explosion.ogg");
        explosionAudio->Play();
        explosionAudio->SetVolume((u32)volume);
        explosionAudio->QueueDestroy();

        if (!m_parent->IsSingleplayer())
            player->TakeDamage();

        m_parent->QueueUnregisterObject(this);
        // only send explosion command if the owner of the landmine detects it
        if (player->IsSelf() && player->GetPlayerId() == m_owner) {
            m_parent->GetClient()->SendSyncedEvent(GameClient::SynchronizedEvent::EVENT_TYPE::EXPLOSION, GetPosition(), 50.0f, 0.007f);
        }
    };

    // check if nearby players that aren't it's owner are in range
    for (auto& playerIt : m_parent->GetPlayers()) {
        if (m_parent->IsSingleplayer()) {
            if (playerIt.second->GetPosition().Distance(this->GetPosition()) > 120.0f) {
                return triggerExplosion(playerIt.second);
            }
        }
        else {
            if (playerIt.first != this->m_owner && !playerIt.second->IsSpectating() && playerIt.second->GetPosition().Distance(this->GetPosition()) < 120.0f) {
                return triggerExplosion(playerIt.second);
            }
        }
    }

    // check if landmine is in lava
    Map* map = m_parent->GetMap();
    if (DoesAABBCollide(m_aabb, [map](Vector2f cornerPos) {
        return map->DoesPixelCollideWithType((s32) cornerPos.x, (s32) cornerPos.y, MAP_PIXEL_TYPE::LAVA);
    })) {
        return triggerExplosion(m_parent->GetPlayer());
    }
}

Vector2f Landmine::GetPosition() {
    return m_aabb.GetCenter();
}

void Landmine::Draw(u32 layerIndex) {
    Render::RenderSpritePortion(s_landmineSprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, (s32)(m_animationIdx/10)*36, 0, 36, 28);
}