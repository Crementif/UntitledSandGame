#pragma once

#include "../common/types.h"
#include "Object.h"

class Player: public Object
{
public:
    Player(GameScene* parent, u32 playerId, f32 posX, f32 posY);
    ~Player() override;

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    void UpdatePosition(const Vector2f& newPos) override;
    void SyncMovement(Vector2f pos, Vector2f speed, bool isDrilling, f32 drillAngle);

    s32 GetPlayerWidth() const;
    s32 GetPlayerHeight() const;

    u32 GetPlayerHealth() const { return m_health; }
    u32 TakeDamage(u8 damage = 1) {
        if (m_health > damage)
            m_health -= damage;
        else
            m_health = 0;

        if (m_health == 0)
            CriticalErrorHandler("Player died!");
        return m_health;
    }
    Vector2f GetPosition() override;
    Vector2f GetSpeed() { return m_speed; }
    bool IsDrilling() { return m_isDrilling; }
    f32 GetDrillAngle() { return m_drillAngle; }

    bool SlidePlayerPos(const Vector2f& newPos); // move player to new position, take collisions into account
    bool DoesPlayerCollideAtPos(f32 posX, f32 posY);

    void HandleLocalPlayerControl();

    bool FindAdjustedGroundHeight(f32 posX, f32 posY, f32& groundHeight, bool& isStuckInGround, bool& isFloatingInAir);
private:
    void HandleLocalPlayerControl_WalkMode(struct ButtonState& buttonState, Vector2f leftStick);
    void HandleLocalPlayerControl_DrillMode(struct ButtonState& buttonState, Vector2f leftStick);

    void Update_DrillMode(float timestep);

    u32 m_playerId;

    AABB CalcAABB(f32 posX, f32 posY);

    Vector2f m_pos;
    Vector2f m_speed{};
    bool m_isTouchingGround{};

    u32 m_health = 10;

    f32 m_moveAnimRot = 0;

    u32 m_drillAnimIdx = 0;
    f32 m_drillAngle{};
    f32 m_visualDrillAngle{};

    // drilling action
    bool m_isDrilling{false};
    f32 m_drillingDur{0.0f};
    //uint8_t m_isDrilling{}; // slowly ramps up
};
