#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"
#include "Map.h"

class Particle: public Object {
public:
    Particle(GameScene* parent, std::unique_ptr<Sprite> sprite, u32 tilesNr, Vector2f pos, u32 rays, f32 distance, u32 animTickSpeed, float randomness = 0.0f) : Object(parent, AABB(pos, Vector2f(0.0000001, 0.0000001)), true, DRAW_LAYER_1), m_distance(distance), m_timePerTile(animTickSpeed), m_sprite(std::move(sprite)), m_tilesNr(tilesNr) {
        for (u32 i = 0; i < rays; i++) {
            f32 angle = (f32)i * (M_TWOPI) / (f32)rays;
            if (randomness > 0.0f) {
                angle += static_cast<float>(rand())/(static_cast<float>((f32)RAND_MAX/(randomness / 360 * M_PI_2)));
            }
            m_directions.emplace_back(Vector2f::FromAngle(angle));
        }
    };
private:
    void Draw(u32 layerIndex) override {
        u32 tileIdx = m_currStep / m_timePerTile;
        u32 tileOffset = (m_sprite->GetWidth()/m_tilesNr);
        for (Vector2f& dir : m_directions) {
            Vector2f emitPos = m_aabb.pos + ((dir * m_distance) * (f32)m_currStep);
            Render::RenderSpritePortion(m_sprite.get(), (s32)emitPos.x * MAP_PIXEL_ZOOM, (s32)emitPos.y * MAP_PIXEL_ZOOM, (s32)tileIdx*tileOffset, 0, 110, 110);
        }
    };
    void Update(float timestep) override {
        if (m_currStep < (m_timePerTile*(m_tilesNr+1)-1)) {
            m_currStep++;
            for (Vector2f& dir : m_directions) {
                Vector2f emitPos = m_aabb.pos + ((dir * m_distance) * (f32)m_currStep);
                Explode(emitPos);
            }
        }
        else
            m_parent->QueueUnregisterObject(this);
    };
    virtual void Explode(Vector2f pos) {};
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };

    const f32 m_distance = 0;
    const u32 m_timePerTile = 0;
    const std::unique_ptr<Sprite> m_sprite;
    const u32 m_tilesNr = 0;
    u32 m_currStep = 0;
    std::vector<Vector2f> m_directions;
};

class BlackholeParticle: public Particle {
public:
    BlackholeParticle(GameScene* parent, Vector2f pos, u32 rays, f32 distance, u32 lifetimeSteps, float randomness) : Particle(parent, std::make_unique<Sprite>("/tex/blackhole0.tga", true), 1, pos, rays, distance, lifetimeSteps, randomness) {
    };
private:
    void Explode(Vector2f pos) override {
    };
};

class ExplosiveParticle: public Particle {
public:
    ExplosiveParticle(GameScene* parent, Vector2f pos, u32 rays, f32 distance, u32 lifetimeSteps, float randomness, float force) : Particle(parent, std::make_unique<Sprite>("/tex/explosion.tga", true), 11, pos, rays, distance, lifetimeSteps, randomness), m_force(force) {
    };
private:
    void Explode(Vector2f pos) override {
    };

    float m_force = 0.0f;
};