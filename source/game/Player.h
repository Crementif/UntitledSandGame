#pragma once

#include "../common/types.h"
#include "Object.h"

class Player : public Object
{
public:
    Player(f32 posX, f32 posY);
    ~Player();

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    void UpdatePosition(const Vector2f& newPos) override;

    s32 GetPlayerWidth() const;
    s32 GetPlayerHeight() const;

    Vector2f GetPosition() override;

    void SlidePlayerPos(const Vector2f& newPos); // move player to new position, take collisions into account
    bool DoesPlayerCollideAtPos(f32 posX, f32 posY);
private:
    AABB CalcAABB(f32 posX, f32 posY);

    void HandleLocalPlayerControl();

    Vector2f m_pos;
    Vector2f m_speed{};
    bool m_isTouchingGround{};
};
