#pragma once
#include "../framework/multiplayer/multiplayer.h"
#include "NetCommon.h"

class GameServer
{
public:
    GameServer();
    ~GameServer();

    void StartGame();

    void Update();

    bool HasGameStarted() const {
        return m_gameStarted;
    }

    u32 GetPlayerCount() const {
        return m_server->GetPlayerCount();
    }

    u32 GetPlayerId(u32 playerIndex) const {
        return m_server->GetPlayerId(playerIndex);
    }

private:
    void ProcessPacket(u32 playerId, u8 opcode, PacketParser& pp);
    bool ProcessPacket_Movement(u32 playerId, PacketParser& pp);
    bool ProcessPacket_Ability(u32 playerId, PacketParser& pp);
    bool ProcessPacket_Drilling(u32 playerId, PacketParser& pp);
    bool ProcessPacket_SyncedEvent(u32 playerId, PacketParser& pp);
    bool ProcessPacket_Pick(u32 playerId, PacketParser &pp);

    RelayServer* m_server;
    bool m_gameStarted{false};
    u32 m_levelId;

};