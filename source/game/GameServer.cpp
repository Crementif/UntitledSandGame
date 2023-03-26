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
    MP_OSReport("[Server] GameServer::StartGame()\n");
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
    MP_OSReport("[Server] GameServer::ProcessPacket playerId %08x opcode %d\n", playerId, (int)opcode);
    bool r = false;
    switch (opcode)
    {
        case NET_ACTION_C_MOVEMENT:
            r = ProcessPacket_Movement(playerId, pp);
            break;
        case NET_ACTION_C_ABILITY:
            r = ProcessPacket_Ability(playerId, pp);
            break;
        case NET_ACTION_C_DRILLING:
            r = ProcessPacket_Drilling(playerId, pp);
            break;
        case NET_ACTION_C_SYNCED_EVENT:
            r = ProcessPacket_SyncedEvent(playerId, pp);
            break;
        case NET_ACTION_C_PICK:
            r = ProcessPacket_Pick(playerId, pp);
            break;
    }
    if(!r)
        OSReport("[Server] GameServer::ProcessPacket: Error processing packet opcode %d from player %08x\n", (int)opcode, playerId);
}

bool GameServer::ProcessPacket_Movement(u32 playerId, PacketParser& pp)
{
    f32 posX = pp.ReadF32();
    f32 posY = pp.ReadF32();
    f32 speedX = pp.ReadF32();
    f32 speedY = pp.ReadF32();
    u8 moveFlags = pp.ReadU8();
    f32 drillAngle = pp.ReadF32();
    // rebuild packet but with playerId
    PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_MOVEMENT);
    pb.AddU32(playerId);
    pb.AddF32(posX);
    pb.AddF32(posY);
    pb.AddF32(speedX);
    pb.AddF32(speedY);
    pb.AddU8(moveFlags);
    pb.AddF32(drillAngle);
    m_server->SendToAllExceptPlayerId(pb, playerId);
    return true;
}

bool GameServer::ProcessPacket_Ability(u32 playerId, PacketParser& pp)
{
    u32 abilityId = pp.ReadU32();
    f32 posX = pp.ReadF32();
    f32 posY = pp.ReadF32();
    f32 velX = pp.ReadF32();
    f32 velY = pp.ReadF32();
    // rebuild packet but with playerId
    PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_ABILITY);
    pb.AddU32(playerId);
    pb.AddU32(abilityId);
    pb.AddF32(posX);
    pb.AddF32(posY);
    pb.AddF32(velX);
    pb.AddF32(velY);
    m_server->SendToAll(pb);
    return true;
}

bool GameServer::ProcessPacket_Drilling(u32 playerId, PacketParser& pp)
{
    f32 posX = pp.ReadF32();
    f32 posY = pp.ReadF32();
    // rebuild packet but with playerId
    PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_DRILLING);
    pb.AddU32(playerId);
    // todo - frame synchronization
    pb.AddF32(posX);
    pb.AddF32(posY);
    m_server->SendToAll(pb);
    return true;
}

bool GameServer::ProcessPacket_Pick(u32 playerId, PacketParser& pp)
{
    f32 posX = pp.ReadF32();
    f32 posY = pp.ReadF32();
    // rebuild packet but with playerId
    PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTION_S_PICK);
    pb.AddU32(playerId);
    pb.AddF32(posX);
    pb.AddF32(posY);
    m_server->SendToAll(pb);
    return true;
}

bool GameServer::ProcessPacket_SyncedEvent(u32 playerId, PacketParser& pp)
{
    u32 eventId = pp.ReadU32();
    f32 posX = pp.ReadF32();
    f32 posY = pp.ReadF32();
    f32 extraParam1 = pp.ReadF32();
    f32 extraParam2 = pp.ReadF32();
    // rebuild packet but with playerId
    PacketBuilder& pb = m_server->BuildNewPacket(NET_ACTIONS_S_SYNCED_EVENT);
    pb.AddU32(playerId);
    // todo - frame synchronization
    pb.AddU32(eventId);
    pb.AddF32(posX);
    pb.AddF32(posY);
    pb.AddF32(extraParam1);
    pb.AddF32(extraParam2);
    m_server->SendToAll(pb);
    return true;
}
