#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr int PORT = 8080;

struct Message {
    size_t bytes;
    std::vector<unsigned char> buffer;
};

constexpr size_t PACKET_SIZE = 1024;

enum class PacketType : uint8_t {
    CONNECT,
    UPDATE,
    DISCONNECT,
};

struct PacketHeader {
    uint32_t timestamp;
    PacketType type;
};

// PROTOCOLS
// update: position/status update
// disconnect: client disconnects, server broadcasts message
struct UpdatePacket {
    PacketHeader header;
    int8_t idx = 0;
    float x, y;
};

#endif // NETWORK_HPP
