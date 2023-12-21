#include "Blackhole.h"
#include "Map.h"
#include "Player.h"
#include "Particle.h"
#include "../framework/audio.h"

Blackhole::Blackhole(GameScene *parent, u32 owner, float x, float y, float volX, float volY) : PhysicsObject(parent, AABB({x, y}, {(f32)32/MAP_PIXEL_ZOOM/4, (f32)32/MAP_PIXEL_ZOOM/4}), DRAW_LAYER_0), m_owner(owner) {
    // the blackhole will have a constant velocity
    this->SetVelocity(volX, volY);
    m_velocityAngle = Vector2f(0.0f, 0.0f).AngleTowards(m_velocity)+(float)M_PI_2;
}

void Blackhole::Update(float timestep) {
    m_animationIdx++;
    if (m_animationIdx >= 45)
        m_animationIdx = 0;

    auto triggerExplosion = [this](Player* player) {
        new ExplosiveParticle(m_parent, std::make_unique<Sprite>("/tex/explosion.tga", true), 11, Vector2f(m_aabb.GetTopLeft().x-11.0f, m_aabb.GetTopLeft().y-11.0f), 8, 1.6f, 2, 20.0f, 20.0f);

        float distance = (m_parent->GetPlayer()->GetPosition().Distance(this->m_aabb.GetCenter())+0.00000001f)/20.0f;
        float volume = 20.0f - (distance/100.0f*20.0f);

        Audio* explosionAudio = new Audio("/sfx/explosion.wav");
        explosionAudio->Play();
        explosionAudio->SetVolume((u32)volume);
        explosionAudio->QueueDestroy();


        for (const auto& nearbyPlayer : m_parent->GetPlayers()) {
            if (nearbyPlayer.second->GetPosition().Distance(this->GetPosition()) < 100.0f && !nearbyPlayer.second->IsSpectating()) {
                nearbyPlayer.second->TakeDamage();
            }
        }

        m_parent->QueueUnregisterObject(this);
        // only send explosion command if the owner of the landmine detects it
        if (player->IsSelf() && player->GetPlayerId() == m_owner) {
            m_parent->GetClient()->SendSyncedEvent(GameClient::SynchronizedEvent::EVENT_TYPE::IMPLOSION, GetPosition(), 40.0f, 0.0f);
        }
    };

    this->SimulatePhysics();
    // check if velocity is zero which means that there was a collision and thus it should explode
    if (this->m_velocity.x == 0.0 && this->m_velocity.y == 0.0) {
        return triggerExplosion(m_parent->GetPlayer());
    }

    // check if missile is in lava
    Map* map = m_parent->GetMap();
    if (DoesAABBCollide(m_aabb, [map](Vector2f cornerPos) { return map->DoesPixelCollideWithType((s32) cornerPos.x, (s32) cornerPos.y, MAP_PIXEL_TYPE::LAVA); })) {
        return triggerExplosion(m_parent->GetPlayer());
    }
}

Vector2f Blackhole::GetPosition() {
    return m_aabb.GetCenter();
}

void Blackhole::Draw(u32 layerIndex) {
    if (m_animationIdx >= 30) {
        Render::RenderSprite(s_blackhole2Sprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, 32, 32, m_velocityAngle);
    }
    else if (m_animationIdx >= 15) {
        Render::RenderSprite(s_blackhole1Sprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, 32, 32, m_velocityAngle);
    }
    else {
        Render::RenderSprite(s_blackhole0Sprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, 32, 32, m_velocityAngle);
    }
}


/*
for (auto& player : m_parent->GetPlayers()) {
    Vector2f direction = GetPosition() - player.second->GetPosition();
    float distance = direction.Length();
    Vector2f force = direction.Normalized() * (m_gravity / (distance * distance));
    player.second->AddForce(force);
}
*/