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
    MP_OSReport("[Client] GameClient::ProcessPacket: Received opcode 0x%02x\n", (int)opcode);
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
        case NET_ACTION_S_PICK:
        {
            r = ProcessPacket_Pick(pp);
            break;
        }
        case NET_ACTIONS_S_SYNCED_EVENT:
        {
            r = ProcessPacket_SyncedEvent(pp);
            break;
        }
    }
    if(!r)
        OSReport("[Client] GameClient::ProcessPacket: Received bad packet 0x%02x\n", (int)opcode);
}

bool GameClient::ProcessPacket_Start(PacketParser& pp)
{
    m_gameInfo.levelId = pp.ReadU32();
    m_gameInfo.rngSeed = pp.ReadU32();
    m_gameInfo.ourPlayerId = pp.ReadU32();
    MP_OSReport("[Client] GameClient::ProcessPacket_Start: Our player id: %08x\n", m_gameInfo.ourPlayerId);
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
    GAME_ABILITY ability = (GAME_ABILITY)pp.ReadU32();
    Vector2f pos;
    pos.x = pp.ReadF32();
    pos.y = pp.ReadF32();
    Vector2f velocity;
    velocity.x = pp.ReadF32();
    velocity.y = pp.ReadF32();
    m_queuedEvents.queueAbility.push_back({playerId, ability, pos, velocity});
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

bool GameClient::ProcessPacket_Pick(PacketParser &pp)
{
    PlayerID playerId = pp.ReadU32();
    Vector2f pos;
    pos.x = pp.ReadF32();
    pos.y = pp.ReadF32();
    m_queuedEvents.queuePickingEvents.emplace_back(EventPick{playerId, pos});
    return true;
}

bool GameClient::ProcessPacket_SyncedEvent(PacketParser &pp)
{
    PlayerID playerId = pp.ReadU32();
    Vector2f pos;
    u32 eventId = pp.ReadU32();
    pos.x = pp.ReadF32();
    pos.y = pp.ReadF32();
    f32 extraParam1 = pp.ReadF32();
    f32 extraParam2 = pp.ReadF32();
    SynchronizedEvent se{};
    SynchronizedEvent::EVENT_TYPE evt = (SynchronizedEvent::EVENT_TYPE)eventId;
    se.eventType = evt;
    se.frameIndex = 9999;
    switch(evt)
    {
        // drilling could be unified with this
        /*
        case SynchronizedEvent::EVENT_TYPE::DRILLING:
        {
            se.action_drill.playerId = playerId;
            se.action_drill.pos.x = pos.x;
            se.action_drill.pos.y = pos.y;
            m_queuedEvents.queueSynchronizedEvents.emplace_back(se);
        }
         */
        case SynchronizedEvent::EVENT_TYPE::EXPLOSION:
        {
            se.action_explosion.playerId = playerId;
            se.action_explosion.pos.x = pos.x;
            se.action_explosion.pos.y = pos.y;
            se.action_explosion.radius = extraParam1;
            se.action_explosion.force = extraParam2;
            m_queuedEvents.queueSynchronizedEvents.emplace_back(se);
            break;
        }
        case SynchronizedEvent::EVENT_TYPE::GRAVITY:
        {
            se.action_gravity.playerId = playerId;
            se.action_gravity.pos.x = pos.x;
            se.action_gravity.pos.y = pos.y;
            se.action_gravity.strength = extraParam1;
            se.action_gravity.lifetimeTicks = extraParam2;
        }
        default:
            break;
    }
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

std::vector<GameClient::EventPick> GameClient::GetAndClearPickingEvents()
{
    std::vector<EventPick> tmp;
    std::swap(tmp, m_queuedEvents.queuePickingEvents);
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

void GameClient::SendPickAction(Vector2f pos)
{
    auto& pb = m_client->BuildNewPacket(NET_ACTION_C_PICK);
    pb.AddF32(pos.x);
    pb.AddF32(pos.y);
    m_client->SendPacket(pb);
}

void GameClient::SendSyncedEvent(SynchronizedEvent::EVENT_TYPE eventType, Vector2f pos, f32 extraParam1, f32 extraParam2)
{
    auto& pb = m_client->BuildNewPacket(NET_ACTION_C_SYNCED_EVENT);
    pb.AddU32((u32)eventType);
    pb.AddF32(pos.x);
    pb.AddF32(pos.y);
    pb.AddF32(extraParam1);
    pb.AddF32(extraParam2);
    m_client->SendPacket(pb);
}
