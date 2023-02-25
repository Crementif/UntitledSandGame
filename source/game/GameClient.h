#pragma once
#include "../framework/multiplayer/multiplayer.h"
#include "NetActions.h"

class GameClient
{
public:
    enum class GAME_STATE : u8
    {
        STATE_LOBBY = 0,
        STATE_INGAME = 1,
    };

public:
    GameClient(std::string_view address);
    ~GameClient();

    bool IsConnected() const { return !m_isConnecting; }
    GAME_STATE GetGameState() { return m_gameState; }

    void Update();
private:
    void ProcessPacket(u8 opcode, PacketParser& pp);

    RelayClient* m_client;
    bool m_isConnecting{false};
    GAME_STATE m_gameState{GAME_STATE::STATE_LOBBY};
};