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
        BOMB = 2,
        TURBO_DRILL = 3,
        LAVA = 4,
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
    };

    struct EventAbility
    {
        PlayerID playerId;
        GAME_ABILITY ability;
        Vector2f pos;
        Vector2f velocity;
    };

    void SendLandmine();

public:
    GameClient(std::string_view address);
    ~GameClient();

    bool IsConnected() const { return !m_isConnecting; }
    GAME_STATE GetGameState() { return m_gameState; }
    const GameSessionInfo& GetGameSessionInfo() { return m_gameInfo; }

    void Update();

    void SendMovement(Vector2f pos, Vector2f speed);
    void SendAbility(GAME_ABILITY ability, Vector2f pos, Vector2f velocity);

    std::vector<EventMovement> GetAndClearMovementEvents();
    std::vector<EventAbility> GetAndClearAbilityEvents();
private:
    void ProcessPacket(u8 opcode, PacketParser& pp);

    bool ProcessPacket_Start(PacketParser& pp);
    bool ProcessPacket_Movement(PacketParser& pp);
    bool ProcessPacket_Ability(PacketParser &pp);

    RelayClient* m_client;
    bool m_isConnecting{false};
    GAME_STATE m_gameState{GAME_STATE::STATE_LOBBY};

    // game info
    GameSessionInfo m_gameInfo;

    // queued events
    struct {
        std::vector<EventMovement> queueMovement;
        std::vector<EventAbility> queueAbility;
    } m_queuedEvents;
};