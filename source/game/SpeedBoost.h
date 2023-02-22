#pragma once
#include "Object.h"
#include "../framework/render.h"

class SpeedBoost : public Object
{
public:
    SpeedBoost(f32 posX, f32 posY);
    ~SpeedBoost();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    bool CanDrag() override { return true; }
    Vector2f GetPosition() override;
    void UpdatePosition(const Vector2f& newPos) override;

private:
    static AABB CalcAABB(Vector2f pos);

    uint8_t spriteIndex = 0;
    float timeDeltaLeft = 0.25;
    std::array<Sprite*, 4> sprites = {
            nullptr,
            nullptr,
            nullptr,
            nullptr
    };
};