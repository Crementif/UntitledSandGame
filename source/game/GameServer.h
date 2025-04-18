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

    static std::string GetLocalIPAddress() {
        char hostName[256];
        if (gethostname(hostName, sizeof(hostName)) != 0) {
            return "";
        }

        struct addrinfo hints;
        struct addrinfo *result = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostName, nullptr, &hints, &result) != 0 || !result) {
            return "";
        }

        bool foundValidAddress = false;
        char ipAddress[INET_ADDRSTRLEN];

        for (struct addrinfo *curr = result; curr != nullptr; curr = curr->ai_next) {
            struct sockaddr_in *addr = (struct sockaddr_in *) curr->ai_addr;
            if (inet_ntop(AF_INET, &(addr->sin_addr), ipAddress, INET_ADDRSTRLEN)) {
                std::string ip(ipAddress);
                if (ip != "127.0.0.1" && ip != "0.0.0.0") {
                    foundValidAddress = true;
                    break;
                }
            }
        }

        freeaddrinfo(result);
        return foundValidAddress ? std::string(ipAddress) : "";
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