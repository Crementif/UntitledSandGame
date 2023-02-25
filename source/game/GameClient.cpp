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
    OSReport("GameClient: Received opcode 0x%02x\n", opcode);
    switch(opcode)
    {
        case NET_ACTION_S_START_GAME:
        {
            // parse packet - todo
            m_gameState = GAME_STATE::STATE_INGAME;
            break;
        }

    }
}