#pragma once
#include "../../common/common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

template <class To, class From>
To _bit_cast(const From& src) noexcept
{
    To dst;
    memcpy(&dst, &src, sizeof(To));
    return dst;
}

enum class GameActionType {
    GameStart,
    PlayerInfo
};

struct GameAction {
    u32 packetSize;
    u32 originPlayer;
};

class PacketBuilder
{
public:
    void Begin(uint8_t opcode)
    {
        m_buffer.resize(3);
        m_buffer[0] = opcode;
    }

    void Finalize()
    {
        u16 packetLength = m_buffer.size() - 3;
        m_buffer[1] = (packetLength>>8)&0xFF;
        m_buffer[2] = (packetLength>>0)&0xFF;
    }

    void AddU32(u32 v)
    {
        m_buffer.emplace_back((v>>24)&0xFF);
        m_buffer.emplace_back((v>>16)&0xFF);
        m_buffer.emplace_back((v>>8)&0xFF);
        m_buffer.emplace_back((v>>0)&0xFF);
    }

    void AddU8(u8 v)
    {
        m_buffer.emplace_back(v);
    }

    void AddF32(f32 v)
    {
        u32 uv = _bit_cast<u32>(v);
        AddU32(uv);
    }

    const std::vector<u8>& GetData() const
    {
        return m_buffer;
    }

private:
    std::vector<u8> m_buffer;
};

class PacketParser
{
public:
    // excludes header
    PacketParser(u8* ptr, u16 size) : m_start(ptr), m_end(m_start + size), m_ptr(ptr) {}

    u32 ReadU32()
    {
        if((m_end - m_ptr) < 4)
            return 0;
        u32 v = 0;
        v |= ((u32)m_ptr[0] << 24);
        v |= ((u32)m_ptr[1] << 16);
        v |= ((u32)m_ptr[2] << 8);
        v |= ((u32)m_ptr[3] << 0);
        m_ptr += 4;
        return v;
    }

    u8 ReadU8()
    {
        if((m_end - m_ptr) < 1)
            return 0;
        u8 v = m_ptr[0];
        m_ptr += 1;
        return v;
    }

    f32 ReadF32()
    {
        u32 v = ReadU32();
        return _bit_cast<f32>(v);
    }

private:
    u8* m_start;
    u8* m_end;
    u8* m_ptr;
};

class RelayServer {
public:
    RelayServer() = default;
    ~RelayServer();
    void SetPacketHandler(void* customPtr, void(*packetHandler)(void* customParam, u32 playerId, u8 opcode, PacketParser& pp))
    {
        cb_PacketHandler = packetHandler;
        m_customParam = customPtr;
    }

    bool AcceptConnections();
    u32 HandleConnectingPlayers();

    u32 GetPlayerCount()
    {
        return (u32)clients.size();
    }

    u32 GetPlayerId(u32 index)
    {
        return clients[index].playerId;
    }

    PacketBuilder& BuildNewPacket(u8 opcode)
    {
        m_pb.Begin(opcode);
        return m_pb;
    }

    void SendToAll(PacketBuilder& pb);
    void SendToPlayerId(PacketBuilder& pb, u32 playerId);
    void SendToAllExceptPlayerId(PacketBuilder& pb, u32 playerId);

    void Update();

    struct Client {
        struct sockaddr_in addr{};
        int socket{};
        u32 playerId;
        std::vector<u8> recvBuffer;
        u32 recvIndex{};
    };
    std::vector<Client> clients;
private:
    struct sockaddr_in receiving_addr{};
    int receiving_socket{};
    PacketBuilder m_pb;
    u32 m_playerIdGen{0};

    void(*cb_PacketHandler)(void* customParam, u32 playerId, u8 opcode, PacketParser& pp){};
    void* m_customParam{};
};

class RelayClient {
public:
    RelayClient() = default;
    ~RelayClient();
    void SetPacketHandler(void* customPtr, void(*packetHandler)(void* customParam, u8 opcode, PacketParser& pp))
    {
        cb_PacketHandler = packetHandler;
        m_customParam = customPtr;
    }

    bool ConnectTo(std::string_view address);
    bool IsConnected() const;

    void Update();

    PacketBuilder& BuildNewPacket(u8 opcode)
    {
        m_pb.Begin(opcode);
        return m_pb;
    }

    void SendPacket(PacketBuilder& pb);

private:
    struct sockaddr_in destination_addr{};
    int conn_socket{};

    std::vector<u8> m_sendBuffer;
    PacketBuilder m_pb;

    std::vector<u8> m_recvBuffer;
    u32 m_recvIndex{0};

    void(*cb_PacketHandler)(void* customParam, u8 opcode, PacketParser& pp){};
    void* m_customParam{};
};