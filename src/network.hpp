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

struct Packet {
    uint32_t timestamp;
    PacketType type;
    char payload[PACKET_SIZE];
};

#endif // NETWORK_HPP
