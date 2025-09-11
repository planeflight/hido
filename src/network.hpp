#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <netinet/in.h>

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
struct ClientPacket {
    PacketHeader header;
    int8_t idx = 0;
    float x, y;
};

struct BulletPacket {
    float x, y;
};
// notifies type/size of next packet
struct SizePacket {
    size_t size = 0;
    enum class IncomingType : uint8_t {
        CLIENT,
        BULLET,
    } type;
};

bool send_packet(int sock,
                 sockaddr_in &client_addr,
                 SizePacket &size_packet,
                 void *packet);

#endif // NETWORK_HPP
