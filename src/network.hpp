#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <netinet/in.h>

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr int PORT = 8080;
constexpr size_t ETHERNET_MTU = 1500;

enum class PacketType : uint8_t {
    CLIENT_UPDATE = 0,
    CLIENT_DISCONNECT,
    BULLET
};

struct PacketHeader {
    uint32_t timestamp;
    PacketType type;
    int8_t sender;
    uint16_t size;
};

constexpr size_t MAX_PACKET_SIZE = ETHERNET_MTU - sizeof(PacketHeader);

using Packet = std::array<int8_t, ETHERNET_MTU>;

// PROTOCOLS
// update: position/status update
// disconnect: client disconnects, server broadcasts message
struct ClientPacket {
    PacketHeader header;
    float x, y;
};
struct BulletPacket {
    float x, y;
};

Packet create_client_packet(ClientPacket packet);

PacketHeader *get_header(Packet &packet);

ClientPacket *get_client_packet_data(Packet &packet);

Packet create_bullet_packet(PacketHeader building_header,
                            const std::vector<BulletPacket> &bullets);

void get_bullet_data(Packet &packet, std::vector<BulletPacket> &bullets);

#endif // NETWORK_HPP
