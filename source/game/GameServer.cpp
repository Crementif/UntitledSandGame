#include "GameServer.h"
#include <coreinit/debug.h>
#include <coreinit/time.h>

GameServer::GameServer()
{
    m_server = new RelayServer();
    m_server->AcceptConnections();

    m_server->SetPacketHandler(this, [](void* customParam, u32 playerId, u8 opcode, PacketParser& pp) {
        GameServer* gs = (GameServer*)customParam;
        gs->ProcessPacket(playerId, opcode, pp);
    });
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
    // generate a seed
    u32 rngSeed = OSGetTick();
    // send every player the start game message
    for(u32 i=0; i<GetPlayerCount(); i++)
    {
        PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_START_GAME);
        pb.AddU32(m_levelId); // levelId
        pb.AddU32(rngSeed); // seed
        pb.AddU32(GetPlayerId(i)); // receiver's own player id
        // attach list of all participating player ids
        pb.AddU32(GetPlayerCount());
        for(u32 f=0; f<GetPlayerCount(); f++)
            pb.AddU32(GetPlayerId(f));
        // send it
        m_server->SendToPlayerId(pb, GetPlayerId(i));
    }
    m_gameStarted = true;
}

void GameServer::Update()
{
    if(!m_gameStarted)
    {
        m_server->HandleConnectingPlayers();
        return;
    }
    m_server->Update();
}

void GameServer::ProcessPacket(u32 playerId, u8 opcode, PacketParser& pp)
{
    OSReport("GameServer::ProcessPacket playerId %08x opcode %d\n", playerId, (int)opcode);
    bool r = false;
    switch(opcode)
    {
        case NET_ACTION_C_MOVEMENT:
            r = ProcessPacket_Movement(playerId, pp);
            break;
    }
    if(!r)
        OSReport("GameServer::ProcessPacket: Error processing packet opcode %d from player %08x\n", (int)opcode, playerId);
}

bool GameServer::ProcessPacket_Movement(u32 playerId, PacketParser& pp)
{
    f32 posX = pp.ReadF32();
    f32 posY = pp.ReadF32();
    f32 speedX = pp.ReadF32();
    f32 speedY = pp.ReadF32();
    // rebuild packet but with playerId
    PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_MOVEMENT);
    pb.AddU32(playerId);
    pb.AddF32(posX);
    pb.AddF32(posY);
    pb.AddF32(speedX);
    pb.AddF32(speedY);
    m_server->SendToAllExceptPlayerId(pb, playerId);
    return true;
}