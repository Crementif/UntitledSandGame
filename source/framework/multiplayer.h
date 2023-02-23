#pragma once
#include "../common/common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct GameAction {
    u32 packetSize;
    int32_t originPlayer;
};


class RelayServer {
public:
    RelayServer() = default;
    ~RelayServer();

    bool AcceptConnections();
    u32 GetConnectedPlayers();

    struct Client {
        struct sockaddr_in addr{};
        int socket{};
    };
    std::vector<Client> clients;
private:
    struct sockaddr_in receiving_addr{};
    int receiving_socket{};
};

class RelayClient {
public:
    RelayClient() = default;
    ~RelayClient();

    bool ConnectTo(std::string_view address);
    bool IsConnected();
private:
    struct sockaddr_in destination_addr{};
    int conn_socket{};
};