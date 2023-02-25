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
    void SyncMovement(Vector2f pos, Vector2f speed);

    s32 GetPlayerWidth() const;
    s32 GetPlayerHeight() const;

    u32 GetPlayerHealth() const { return m_health; }
    Vector2f GetPosition() override;
    Vector2f GetSpeed() { return m_speed; }

    bool SlidePlayerPos(const Vector2f& newPos); // move player to new position, take collisions into account
    bool DoesPlayerCollideAtPos(f32 posX, f32 posY);

    void HandleLocalPlayerControl();
private:
    AABB CalcAABB(f32 posX, f32 posY);

    Vector2f m_pos;
    Vector2f m_speed{};
    bool m_isTouchingGround{};

    u32 m_health = 3;
};
