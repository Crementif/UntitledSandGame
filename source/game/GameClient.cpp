#include "GameClient.h"
#include <coreinit/debug.h>

GameClient::GameClient(std::string_view address)
{
    m_client = new RelayClient();
    m_client->ConnectTo(address);
    m_isConnecting = true;

    m_client->SetPacketHandler(this, [](void* customParam, u8 opcode, PacketParser& pp) {
        GameClient* gc = (GameClient*)customParam;
        gc->ProcessPacket(opcode, pp);
    });
}

GameClient::~GameClient()
{
}

void GameClient::Update()
{
    if(m_isConnecting)
    {
        if( m_client->IsConnected() )
            m_isConnecting = false;
        return;
    }
    m_client->Update();
}

void GameClient::ProcessPacket(u8 opcode, PacketParser& pp)
{
    OSReport("GameClient: Received opcode 0x%02x\n", (int)opcode);
    bool r = false;
    switch(opcode)
    {
        case NET_ACTION_S_START_GAME:
        {
            r = ProcessPacket_Start(pp);
            break;
        }
        case NET_ACTION_S_MOVEMENT:
        {
            r = ProcessPacket_Movement(pp);
            break;
        }
        case NET_ACTION_S_ABILITY:
        {
            r = ProcessPacket_Ability(pp);
            break;
        }
        case NET_ACTION_S_DRILLING:
        {
            r = ProcessPacket_Drilling(pp);
            break;
        }
    }
    if(!r)
        OSReport("GameClient: Received bad packet 0x%02x\n", (int)opcode);
}

bool GameClient::ProcessPacket_Start(PacketParser& pp)
{
    m_gameInfo.levelId = pp.ReadU32();
    m_gameInfo.rngSeed = pp.ReadU32();
    m_gameInfo.ourPlayerId = pp.ReadU32();
    OSReport("GameClient::ProcessPacket_Start: Our player id: %08x\n", m_gameInfo.ourPlayerId);
    u32 playerCount = pp.ReadU32();
    m_gameInfo.playerIds.clear();
    for(u32 i=0; i<playerCount; i++)
        m_gameInfo.playerIds.push_back(pp.ReadU32());
    // start game
    m_gameState = GAME_STATE::STATE_INGAME;
    return true;
}

bool GameClient::ProcessPacket_Movement(PacketParser& pp)
{
    PlayerID playerId = pp.ReadU32();
    Vector2f pos;
    pos.x = pp.ReadF32();
    pos.y = pp.ReadF32();
    Vector2f speed;
    speed.x = pp.ReadF32();
    speed.y = pp.ReadF32();
    u8 moveFlags = pp.ReadU8();
    f32 drillAngle = pp.ReadF32();
    m_queuedEvents.queueMovement.push_back({playerId, pos, speed, moveFlags, drillAngle});
    return true;
}

bool GameClient::ProcessPacket_Ability(PacketParser& pp)
{
    PlayerID playerId = pp.ReadU32();
    u32 ability = pp.ReadU32();
    Vector2f pos;
    pos.x = pp.ReadF32();
    pos.y = pp.ReadF32();
    Vector2f velocity;
    velocity.x = pp.ReadF32();
    velocity.y = pp.ReadF32();
    m_queuedEvents.queueAbility.push_back({playerId, (GAME_ABILITY)ability, pos, velocity});
    return true;
}

bool GameClient::ProcessPacket_Drilling(PacketParser &pp)
{
    PlayerID playerId = pp.ReadU32();
    Vector2f pos;
    pos.x = pp.ReadF32();
    pos.y = pp.ReadF32();
    SynchronizedEvent se{};
    se.eventType = SynchronizedEvent::EVENT_TYPE::DRILLING;
    se.frameIndex = 9999;
    se.action_drill.playerId = playerId;
    se.action_drill.pos.x = pos.x;
    se.action_drill.pos.y = pos.y;
    m_queuedEvents.queueSynchronizedEvents.emplace_back(se);
    return true;
}

std::vector<GameClient::EventMovement> GameClient::GetAndClearMovementEvents()
{
    std::vector<EventMovement> tmp;
    std::swap(tmp, m_queuedEvents.queueMovement);
    return tmp;
}

std::vector<GameClient::EventAbility> GameClient::GetAndClearAbilityEvents()
{
    std::vector<EventAbility> tmp;
    std::swap(tmp, m_queuedEvents.queueAbility);
    return tmp;
}

bool GameClient::GetSynchronizedEvents(u32 frameCount, std::vector<GameClient::SynchronizedEvent>& events)
{
    events.clear();
    std::swap(events, m_queuedEvents.queueSynchronizedEvents);
    return true;
}

void GameClient::SendMovement(Vector2f pos, Vector2f speed, u8 moveFlags, f32 drillAngle)
{
    auto& pb = m_client->BuildNewPacket(NET_ACTION_C_MOVEMENT);
    pb.AddF32(pos.x);
    pb.AddF32(pos.y);
    pb.AddF32(speed.x);
    pb.AddF32(speed.y);
    pb.AddU8(moveFlags);
    pb.AddF32(drillAngle);
    m_client->SendPacket(pb);
}

void GameClient::SendAbility(GameClient::GAME_ABILITY ability, Vector2f pos, Vector2f velocity)
{
    auto& pb = m_client->BuildNewPacket(NET_ACTION_C_ABILITY);
    pb.AddU32((u32)ability);
    pb.AddF32(pos.x);
    pb.AddF32(pos.y);
    pb.AddF32(velocity.x);
    pb.AddF32(velocity.y);
    m_client->SendPacket(pb);
}

void GameClient::SendDrillingAction(Vector2f pos)
{
    auto& pb = m_client->BuildNewPacket(NET_ACTION_C_DRILLING);
    pb.AddF32(pos.x);
    pb.AddF32(pos.y);
    m_client->SendPacket(pb);
}