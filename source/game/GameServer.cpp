#include "GameServer.h"
#include <coreinit/debug.h>

GameServer::GameServer()
{
    m_server = new RelayServer();
    m_server->AcceptConnections();
}

GameServer::~GameServer()
{
    delete m_server;
}

void GameServer::StartGame()
{
    OSReport("GameServer::StartGame()\n");
    // choose a level
    m_levelId = 0;
    // send every player the start game message
    for(u32 i=0; i<GetPlayerCount(); i++)
    {
        PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_START_GAME);
        pb.AddU32(m_levelId); // levelId
        // attach list of all participating player ids
        pb.AddU32(GetPlayerCount());
        for(u32 f=0; f<GetPlayerCount(); f++)
            pb.AddU32(GetPlayerId(f));
        // send it
        m_server->SendToPlayerId(pb, GetPlayerId(i));
    }
}

void GameServer::Update()
{
    if(!m_gameStarted)
    {
        m_server->HandleConnectingPlayers();
        return;
    }
    else
    {

    }
}
