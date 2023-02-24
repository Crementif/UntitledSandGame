#include <experimental/io_context>
#include "multiplayer.h"

int sockOptionEnable = 1;

bool RelayServer::AcceptConnections() {
    if ((receiving_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) return false;
    setsockopt(receiving_socket, SOL_SOCKET, SO_REUSEADDR | SO_NONBLOCK, &sockOptionEnable, sizeof(sockOptionEnable));

    receiving_addr.sin_port = htons(8890);
    receiving_addr.sin_family = AF_INET;
    receiving_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(receiving_socket, (struct sockaddr*)&receiving_addr, sizeof(receiving_addr)) < 0) return false;

    if (listen(receiving_socket, 1) < 0) return false;
    return true;
};

RelayServer::~RelayServer() {
    for (auto& client : clients) {
        close(client.socket);
    }
    close(receiving_socket);
};

u32 RelayServer::GetConnectedPlayers() {
    // receive incoming connections
    while (true) {
        Client newSocket;
        constexpr socklen_t sockAddrSize = sizeof(struct sockaddr_in);
        if ((newSocket.socket = accept(receiving_socket, (struct sockaddr*)&newSocket.addr, (socklen_t*)&sockAddrSize)) < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            OSFatal("Error: Failed to accept an incoming connection");
            exit(EXIT_FAILURE);
        }

        // todo: check if setting non-blocking for client sockets is important
        if (setsockopt(receiving_socket, SOL_SOCKET, SO_NONBLOCK, &sockOptionEnable, sizeof(sockOptionEnable)) < 0)
            continue;

        this->clients.emplace_back(newSocket);
    }

    return this->clients.size();
};

bool RelayClient::ConnectTo(std::string_view address) {
    if ((conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) return false;
    if (setsockopt(conn_socket, SOL_SOCKET, SO_REUSEADDR | SO_NONBLOCK, &sockOptionEnable, sizeof(sockOptionEnable)) < 0) return false;

    destination_addr.sin_family = AF_INET;
    destination_addr.sin_port = htons(8890);
    if (inet_aton(address.data(), &destination_addr.sin_addr) <= 0) return false;

    if (connect(conn_socket, (struct sockaddr *)&destination_addr, sizeof(destination_addr)) < 0) {
        if (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN) {
            return false;
        }
        return false;
    }
    WHBLogPrintf("Connecting to %s", address.data());
    return true;
};

bool RelayClient::IsConnected() const {
    struct sockaddr_in addr = {};
    socklen_t addrLength = sizeof(addr);
    bool ret = getpeername(conn_socket, (struct sockaddr*)&addr, &addrLength) == 0;
    return ret;
}

RelayClient::~RelayClient() {
    close(conn_socket);
};