#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <netinet/in.h>
#include <raylib.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "state/player.hpp"

constexpr int PORT = 8080;
constexpr size_t ETHERNET_MTU = 1500;
constexpr size_t MAX_PLAYERS = 5;
// since packets take time to arrive
constexpr uint64_t INTERPOLATION_DELAY = 100;
constexpr uint32_t TICK_INTERVAL = 1000 / 60;

enum class PacketType : uint8_t {
    CLIENT_DISCONNECT,
    BULLET,
    BULLET_COLLISION,
    INPUT,
    GAME_STATE,
};

struct PacketHeader {
    uint64_t timestamp = 0; // millis
    PacketType type;
    int sender = -1;
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

struct BulletCollisionPacket {
    PacketHeader header;
    int8_t to;
};

struct InputPacket {
    PacketHeader header;
    bool left, right, up, down, mouse_down;
    Vector2 mouse_pos;
};

struct GameStatePacket {
    PacketHeader header;
    int8_t num_players = 0;
    int client_id = 0; // tells clients what their id is
    std::array<PlayerState, MAX_PLAYERS> players;
};

struct BulletPacket {
    int sender = -1, id = -1;
    Vector2 pos;
};
constexpr size_t MAX_BULLETS_PER_PACKET =
    MAX_PACKET_SIZE / sizeof(BulletPacket);

struct BulletStatePacket {
    PacketHeader header;
    int num_bullets = 0;
    std::array<BulletPacket, MAX_BULLETS_PER_PACKET> bullets;
};

/**
 * Create a packet from a packet type such as InputPacket
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

inline uint64_t get_now_millis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

inline uint64_t get_render_time() {
    return get_now_millis() - INTERPOLATION_DELAY;
}

#endif // NETWORK_HPP
