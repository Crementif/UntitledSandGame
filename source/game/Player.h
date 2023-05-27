#pragma once

#include "../common/types.h"
#include "Object.h"

class Player: public Object
{
public:
    union MOVEMENT_BITS
    {
        struct
        {
            bool walkingLeft : 1;
            bool walkingRight : 1;
            bool isDrilling : 1;
        };
        u8 rawBits;
    };
    static_assert(sizeof(MOVEMENT_BITS) == 1);

    Player(GameScene* parent, u32 playerId, f32 posX, f32 posY);
    ~Player() override;

    bool IsSelf() const { return m_isSelf; };
    void SetSelfFlag(bool isSelf) { m_isSelf = isSelf; }
    PlayerID GetPlayerId() const { return m_playerId; }

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    void UpdatePosition(const Vector2f& newPos) override;
    void SyncMovement(Vector2f pos, Vector2f speed, u8 moveFlags, f32 drillAngle);

    s32 GetPlayerWidth() const;
    s32 GetPlayerHeight() const;

    u32 GetPlayerHealth() const { return m_health; }
    u32 TakeDamage(u8 damage = 1);
    bool IsInvincible() const { return m_invincibility >= OSGetTime(); }
    bool IsSpectating() const { return m_spectating; }
    bool IsTurboBoosting() const { return m_turboBoost >= OSGetTime(); }
    void ChangeToSpectator();
    GameClient::GAME_ABILITY GetAbility() const { return m_ability; }
    void GiveAbility(GameClient::GAME_ABILITY ability) { m_ability = ability; }

    Vector2f GetPosition() override;
    Vector2f GetSpeed() { return m_speed; }
    bool IsDrilling() { return m_moveFlags.isDrilling; }
    u8 GetMovementFlags() { return m_moveFlags.rawBits; }
    f32 GetDrillAngle() { return m_drillAngle; }

    bool SlidePlayerPos(class Map* map, Vector2f newPos); // move player to new position, take collisions into account
    bool DoesPlayerCollideAtPos(f32 posX, f32 posY);
    bool CheckCanDrill();

    void HandleLocalPlayerControl();

    bool FindAdjustedGroundHeight(f32 posX, f32 posY, f32& groundHeight, bool& isStuckInGround, bool& isFloatingInAir);
private:
    void HandleLocalPlayerControl_WalkMode(struct ButtonState& buttonState, Vector2f leftStick);
    void HandleLocalPlayerControl_DrillMode(struct ButtonState& buttonState, Vector2f leftStick);
    void HandleLocalPlayerControl_SpectatingMode(struct ButtonState& buttonState, Vector2f leftStick);

    void Update_DrillMode(float timestep);
    void Update_SpectatingMode(float timestep);

    u32 m_playerId;

    AABB CalcAABB(f32 posX, f32 posY);

    Vector2f m_pos;
    Vector2f m_speed{};
    bool m_isTouchingGround{};

    bool m_isSelf{false};
    u32 m_health = 3;
    OSTime m_invincibility = 0;
    bool m_spectating = false;
    u32 m_spectatingPlayerIdx = 0;
    Player* m_spectatingPlayer = nullptr;
    GameClient::GAME_ABILITY m_ability = GameClient::GAME_ABILITY::NONE;
    OSTime m_turboBoost = 0;

    f32 m_moveAnimRot = 0;

    u32 m_drillAnimIdx = 0;
    f32 m_drillAngle{};
    f32 m_visualDrillAngle{};
    // movement flags
    MOVEMENT_BITS m_moveFlags;
    // drilling action
    f32 m_drillingDur{0.0f};
    //uint8_t m_isDrilling{}; // slowly ramps up

    // assets
    class Audio* m_teleportAudio;
    class Audio* m_deathAudio;
    class Audio* m_hitAudio;
    class Audio* m_drillAudio;
};
