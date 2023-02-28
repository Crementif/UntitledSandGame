#include "multiplayer.h"

bool RelayServer::AcceptConnections() {
    if ((receiving_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return false;
    int sockOptionEnable = 1;
    setsockopt(receiving_socket, SOL_SOCKET, SO_NONBLOCK, &sockOptionEnable, sizeof(sockOptionEnable));

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

u32 RelayServer::HandleConnectingPlayers() {
    // receive incoming connections
    while (true) {
        Client newClient;
        constexpr socklen_t sockAddrSize = sizeof(struct sockaddr_in);
        if ((newClient.socket = accept(receiving_socket, (struct sockaddr*)&newClient.addr, (socklen_t*)&sockAddrSize)) < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            OSFatal("Error: Failed to accept an incoming connection");
            exit(EXIT_FAILURE);
        }

        int sockOptionEnable = 1;
        if (setsockopt(newClient.socket, SOL_SOCKET, SO_NONBLOCK, &sockOptionEnable, sizeof(sockOptionEnable)) < 0)
            continue;

        newClient.playerId = m_playerIdGen;
        m_playerIdGen++;
        this->clients.emplace_back(newClient);
    }

    return this->clients.size();
};

void RelayServer::Update()
{
    // check for incoming packets
    // todo - its probably faster if we use select here? But non-blocking recv should be fine for our purposes
    // recvBuffer
    for(auto& client : clients)
    {
        // make sure there is always space in the receive buffer
        u32 minExpectedBufferSize = client.recvIndex + 256;
        if(minExpectedBufferSize > client.recvBuffer.size())
            client.recvBuffer.resize(minExpectedBufferSize);
        // receive as many bytes as fit into the buffer
        u32 remainingBufferBytes = client.recvBuffer.size() - client.recvIndex;
        int r = recv(client.socket, client.recvBuffer.data() + client.recvIndex, remainingBufferBytes, 0);
        if(r <= 0)
            continue;
        OSReport("RelayServer::Update r %d\n", r);
        client.recvIndex += (u32)r;
        // process as many whole packets as available
        while( true ) {
            if (client.recvIndex < 3)
                break;
            u32 packetSize = 0;
            packetSize |= ((u32)client.recvBuffer[1] << 8);
            packetSize |= ((u32)client.recvBuffer[2] << 0);
            packetSize += 3;
            if (client.recvIndex < packetSize)
                break;
            // process packet
            if (cb_PacketHandler)
            {
                u8 opcode = client.recvBuffer[0];
                PacketParser pp(client.recvBuffer.data() + 3, packetSize - 3);
                cb_PacketHandler(m_customParam, client.playerId, opcode, pp);
            }
            // shift data
            client.recvBuffer.erase(client.recvBuffer.begin(), client.recvBuffer.begin() + packetSize);
            client.recvIndex -= packetSize;
        }
    }

}

void RelayServer::SendToAll(PacketBuilder& pb)
{
    pb.Finalize();
    const std::vector<u8>& data = pb.GetData();
    for(auto& it : clients)
    {
        OSReport("[Server] Send %d bytes to player %08x\n", (int)pb.GetData().size(), it.playerId);
        send(it.socket, data.data(), data.size(), 0);
        // todo - handle incomplete sends
    }
}

RelayClient::~RelayClient() {
    close(conn_socket);
};

void RelayServer::SendToPlayerId(PacketBuilder& pb, u32 playerId)
{
    pb.Finalize();
    const std::vector<u8>& data = pb.GetData();
    for(auto& it : clients)
    {
        if(it.playerId == playerId)
        {
            OSReport("[Server] Send %d bytes to player %08x\n", (int)pb.GetData().size(), playerId);
            send(it.socket, data.data(), data.size(), 0);
            return;
        }
    }
}

void RelayServer::SendToAllExceptPlayerId(PacketBuilder& pb, u32 playerId)
{
    pb.Finalize();
    const std::vector<u8>& data = pb.GetData();
    for(auto& it : clients)
    {
        if(it.playerId != playerId)
        {
            OSReport("[Server] Send %d bytes to player %08x\n", (int)pb.GetData().size(), playerId);
            send(it.socket, data.data(), data.size(), 0);
            return;
        }
    }
}


bool RelayClient::ConnectTo(std::string_view address)
{
    if ((conn_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return false;
    int sockOptionEnable = 1;
    if (setsockopt(conn_socket, SOL_SOCKET, SO_NONBLOCK, &sockOptionEnable, sizeof(sockOptionEnable)) < 0)
        return false;

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

void RelayClient::SendPacket(PacketBuilder& pb)
{
    pb.Finalize();
    auto& buf = pb.GetData();
    int r = send(conn_socket, buf.data(), buf.size(), 0);
    if(r != (s32)buf.size())
        OSReport("Failed to send full message");
}

void RelayClient::Update()
{
    if((m_recvIndex + 128) > m_recvBuffer.size())
        m_recvBuffer.resize(m_recvIndex + 128);
    u32 bufferBytesRemaining = (u32)m_recvBuffer.size() - m_recvIndex;
    int r = recv(conn_socket, m_recvBuffer.data() + m_recvIndex, bufferBytesRemaining, 0);
    if(r <= 0)
        return;
    OSReport("[Client] Received %d bytes\n", (int)r);
    m_recvIndex += r;
    while( true )
    {
        if(m_recvIndex < 3)
            return;
        u32 packetSize = 0;
        packetSize |= ((u32)m_recvBuffer[1] << 8);
        packetSize |= ((u32)m_recvBuffer[2] << 0);
        packetSize += 3;
        if(m_recvIndex < packetSize)
            return;
        // process packet
        if(cb_PacketHandler)
        {
            u8 opcode = m_recvBuffer[0];
            PacketParser pp(m_recvBuffer.data() + 3, packetSize-3);
            cb_PacketHandler(m_customParam, opcode, pp);
        }
        // shift data
        m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + packetSize);
        m_recvIndex -= packetSize;
    }
}
