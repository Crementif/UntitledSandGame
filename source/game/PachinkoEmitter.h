#pragma once
#include "Object.h"

class PachinkoEmitter : public Object
{
public:
    PachinkoEmitter(f32 posX, f32 posY);
    ~PachinkoEmitter();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    Vector2f GetPosition() override;

private:
    Vector2f m_pos;
    AABB CalcAABB();
    float m_timer{};
};