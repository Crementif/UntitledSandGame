#pragma once
#include "../framework/multiplayer/multiplayer.h"
#include "NetCommon.h"

class GameClient
{
public:
    enum class GAME_STATE : u8
    {
        STATE_LOBBY = 0,
        STATE_INGAME = 1,
    };

    enum class GAME_ABILITY : u32 {
        NONE = 0,
        LANDMINE = 1,
        MISSILE = 2,
        TURBO_DRILL = 3,
        BOMB = 4,
        LAVA = 5,
        JETPACK = 6,
        DEATH = 7 // special non-collectible ability for the explosion when a player dies
    };

    struct GameSessionInfo
    {
        u32 levelId;
        PlayerID ourPlayerId;
        u32 rngSeed;
        std::vector<PlayerID> playerIds; // deterministic. Always in same order for each player
    };

    struct EventMovement
    {
        PlayerID playerId;
        Vector2f pos;
        Vector2f speed;
        u8 moveFlags;
        f32 drillAngle;
    };

    struct EventAbility
    {
        PlayerID playerId;
        GAME_ABILITY ability;
        Vector2f pos;
        Vector2f velocity;
    };

    struct EventPick
    {
        PlayerID playerId;
        Vector2f pos;
    };

    struct SynchronizedEvent
    {
        enum class EVENT_TYPE : u8
        {
            DRILLING = 0,
            EXPLOSION = 1,
        };
        EVENT_TYPE eventType;
        u32 frameIndex;
        union
        {
            struct
            {
                PlayerID playerId;
                Vector2f pos;
            }action_drill;
            struct
            {
                PlayerID playerId;
                Vector2f pos;
                f32 radius;
            }action_explosion;
        };
    };

public:
    GameClient(std::string_view address);
    ~GameClient();

    bool IsConnected() const { return !m_isConnecting; }
    GAME_STATE GetGameState() { return m_gameState; }
    const GameSessionInfo& GetGameSessionInfo() { return m_gameInfo; }

    void Update();

    void SendMovement(Vector2f pos, Vector2f speed, u8 moveFlags, f32 drillAngle);
    void SendAbility(GAME_ABILITY ability, Vector2f pos, Vector2f velocity);
    void SendDrillingAction(Vector2f pos);
    void SendPickAction(Vector2f pos);
    void SendSyncedEvent(SynchronizedEvent::EVENT_TYPE eventType, Vector2f pos, f32 extraParam1, f32 extraParam2);

    std::vector<EventMovement> GetAndClearMovementEvents();
    std::vector<EventAbility> GetAndClearAbilityEvents();
    std::vector<EventPick> GetAndClearPickingEvents();
    bool GetSynchronizedEvents(u32 frameCount, std::vector<GameClient::SynchronizedEvent>& events);
private:
    void ProcessPacket(u8 opcode, PacketParser& pp);

    bool ProcessPacket_Start(PacketParser& pp);
    bool ProcessPacket_Movement(PacketParser& pp);
    bool ProcessPacket_Ability(PacketParser &pp);
    bool ProcessPacket_Drilling(PacketParser &pp);
    bool ProcessPacket_SyncedEvent(PacketParser &pp);
    bool ProcessPacket_Pick(PacketParser &pp);

    RelayClient* m_client;
    bool m_isConnecting{false};
    GAME_STATE m_gameState{GAME_STATE::STATE_LOBBY};

    // game info
    GameSessionInfo m_gameInfo;

    // queued events
    struct {
        std::vector<EventMovement> queueMovement;
        std::vector<EventAbility> queueAbility;
        std::vector<SynchronizedEvent> queueSynchronizedEvents;
        std::vector<EventPick> queuePickingEvents;
    } m_queuedEvents;
};