#include "network.hpp"

#include <spdlog/spdlog.h>
#include <sys/socket.h>

#include <cstring>

PacketHeader *get_header(Packet &packet) {
    return (PacketHeader *)packet.data();
}
