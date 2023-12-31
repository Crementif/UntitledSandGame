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
    if (!m_imploding) {
        m_shootAnimationIdx++;
        if (m_shootAnimationIdx >= 45)
            m_shootAnimationIdx = 0;

        this->SimulatePhysics();
    }
    else {
        constexpr u32 m_implosionStep = 3;
        constexpr u32 m_implosionStepLength = 15;

        if (m_implosionAnimationIdx == 0) {
            new BlackholeParticle(m_parent, Vector2f(m_aabb.GetTopLeft().x-11.0f, m_aabb.GetTopLeft().y-11.0f), 8, 1.8f*1.5f, 1, 20.0f);

            if (m_parent->GetPlayer()->GetPlayerId() == m_owner) {
                m_parent->GetClient()->SendSyncedEvent(GameClient::SynchronizedEvent::EVENT_TYPE::GRAVITY, GetPosition(), 50.0f, 100.0f);
                m_parent->GetClient()->SendSyncedEvent(GameClient::SynchronizedEvent::EVENT_TYPE::EXPLOSION, GetPosition(), 50.0f, -0.0001f);
            }
        }

        m_implosionAnimationIdx++;
        if (m_implosionAnimationIdx >= (m_implosionStep*m_implosionStepLength)) {
            m_parent->QueueUnregisterObject(this);
        }
    }

    // check if velocity is zero which means that there was a collision and thus the blackhole should implode
    if (!m_imploding && this->m_velocity.x == 0.0 && this->m_velocity.y == 0.0) {
        m_imploding = true;

        float distance = (m_parent->GetPlayer()->GetPosition().Distance(this->m_aabb.GetCenter())+0.00000001f)/20.0f;
        float volume = 20.0f - (distance/100.0f*20.0f);

        Audio* explosionAudio = new Audio("/sfx/explosion.wav");
        explosionAudio->Play();
        explosionAudio->SetVolume((u32)volume);
        explosionAudio->QueueDestroy();
    }
}

Vector2f Blackhole::GetPosition() {
    return m_aabb.GetCenter();
}

void Blackhole::Draw(u32 layerIndex) {
    if (m_shootAnimationIdx >= 30) {
        Render::RenderSprite(s_blackhole2Sprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM, 32, 32, m_velocityAngle);
    }
    else if (m_shootAnimationIdx >= 15) {
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