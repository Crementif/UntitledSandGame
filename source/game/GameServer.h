#pragma once
#include "../framework/multiplayer/multiplayer.h"
#include "NetActions.h"

class GameServer
{
public:
    GameServer();
    ~GameServer();

    void StartGame();

    void Update();

    u32 GetPlayerCount()
    {
        return m_server->GetPlayerCount();
    }

    u32 GetPlayerId(u32 playerIndex)
    {
        return m_server->GetPlayerId(playerIndex);
    }

private:
    RelayServer* m_server;
    bool m_gameStarted{false};
    u32 m_levelId;
};