#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <netinet/in.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "state/player.hpp"

constexpr int PORT = 8080;
constexpr size_t ETHERNET_MTU = 1500;
constexpr size_t MAX_PLAYERS = 5;
// since packets take time to arrive
constexpr uint64_t INTERPOLATION_DELAY = 100;

enum class PacketType : uint8_t {
    CLIENT_UPDATE = 0,
    CLIENT_DISCONNECT,
    BULLET,
    BULLET_COLLISION,
    INPUT,
    GAME_STATE,
};

struct PacketHeader {
    uint64_t timestamp = 0; // millis
    PacketType type;
    int8_t sender = -1;
    uint16_t size; // optional if not a dynamic buffer
};

constexpr size_t MAX_PACKET_SIZE = ETHERNET_MTU - sizeof(PacketHeader);

using Packet = std::array<int8_t, ETHERNET_MTU>;

// PROTOCOLS
// update: position/status update
// disconnect: client disconnects, server broadcasts message
struct ClientPacket {
    PacketHeader header;
    float x, y, health;
};
struct BulletPacket {
    float x, y;
};

struct BulletCollisionPacket {
    PacketHeader header;
    int8_t to;
};

struct InputPacket {
    PacketHeader header;
    bool left, right, up, down, mouse;
};

struct GameStatePacket {
    PacketHeader header;
    int8_t num_players = 0;
    int client_id = 0; // tells clients what their id is
    std::array<PlayerState, MAX_PLAYERS> players;
};

/**
 * Create a packet from a packet type such as ClientPacket,
 * BulletCollisionPacket. Just copies the data
 * */
template <typename T>
Packet create_packet_from(const T &packet) {
    Packet p;
    auto dest = p.data();
    memcpy(dest, &packet, sizeof(packet));
    return p;
}

template <typename T>
T *get_packet_data(Packet &packet) {
    return (T *)packet.data();
}

PacketHeader *get_header(Packet &packet);

Packet create_bullet_packet(PacketHeader building_header,
                            const std::vector<BulletPacket> &bullets);

void get_bullet_data(Packet &packet, std::vector<BulletPacket> &bullets);

#endif // NETWORK_HPP
