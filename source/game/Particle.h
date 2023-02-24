#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"
#include "Map.h"

class Particle: public Object {
public:
    Particle(std::unique_ptr<Sprite> sprite, Vector2f pos, u32 rays, f32 distance, u32 lifetimeSteps, float randomness) : Object(AABB(pos, Vector2f(1, 1)), true, DRAW_LAYER_1), m_distance(distance), m_maxSteps(lifetimeSteps), m_sprite(std::move(sprite)) {
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
        if (m_currStep < m_maxSteps)
            m_currStep++;
        else
            Object::QueueForDeletion(this);
    };
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };

    const f32 m_distance = 0;
    const u32 m_maxSteps = 0;
    const std::unique_ptr<Sprite> m_sprite;
    u32 m_currStep = 0;
    std::vector<Vector2f> m_directions;
};
