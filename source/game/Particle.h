#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"
#include "Map.h"

class Particle: public Object {
public:
    Particle(GameScene* parent, std::unique_ptr<Sprite> sprite, Vector2f pos, u32 rays, f32 distance, u32 lifetimeSteps, float randomness) : Object(parent, AABB(pos, Vector2f(1, 1)), true, DRAW_LAYER_1), m_distance(distance), m_maxSteps(lifetimeSteps), m_sprite(std::move(sprite)) {
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
        for (Vector2f& dir : m_directions) {
            Vector2f emitPos = m_aabb.pos + ((dir * m_distance) * (f32)m_currStep);
            Render::RenderSprite(m_sprite.get(), (s32)emitPos.x * MAP_PIXEL_ZOOM, (s32)emitPos.y * MAP_PIXEL_ZOOM);
        }
    };
    void Update(float timestep) override {
        if (m_currStep < m_maxSteps) {
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
    const u32 m_maxSteps = 0;
    const std::unique_ptr<Sprite> m_sprite;
    u32 m_currStep = 0;
    std::vector<Vector2f> m_directions;
};

class ExplosiveParticle: public Particle {
public:
    ExplosiveParticle(GameScene* parent, std::unique_ptr<Sprite> sprite, Vector2f pos, u32 rays, f32 distance, u32 lifetimeSteps, float randomness, float force) : Particle(parent, std::move(sprite), pos, rays, distance, lifetimeSteps, randomness), m_force(force) {
    };
private:
    void Explode(Vector2f pos) override {
        Map* map = this->m_parent->GetMap();

        AABB explosionRange = AABB({pos.x-(f32)m_force/2, pos.y-(f32)m_force/2}, Vector2f(m_force, m_force));

        // loop over AABB pixels
        for (s32 x = (s32)explosionRange.pos.x; x < (s32)(explosionRange.pos.x + explosionRange.scale.x); x++) {
            for (s32 y = (s32)explosionRange.pos.y; y < (s32)(explosionRange.pos.y + explosionRange.scale.y); y++) {
                // check if map pixel is in range of the explosion force and whether it is solid/destructible
                if (map->IsPixelOOB(x, y))
                    continue;

                PixelType& pixel = map->GetPixelNoBoundsCheck(x, y);
                if (pos.Distance({(f32)x, (f32)y}) <= m_force && pixel.IsSolid() && pixel.IsDestructible()) {
                    // adjust force for distance
                    f32 distanceAdjustedForce = m_force - Vector2f((f32)x, (f32)y).Distance(pos);
                    // apply force to pixel
                    map->ReanimateStaticPixel(pixel.GetPixelType(), x, y, distanceAdjustedForce);
                }
            }
        }
    };

    float m_force = 0.0f;
};