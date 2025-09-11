#include "network.hpp"

#include <spdlog/spdlog.h>
#include <sys/socket.h>

bool send_packet(int sock,
                 sockaddr_in &client_addr,
                 SizePacket &size_packet,
                 void *packet) {
    // send size first
    int n = sendto(sock,
                   &size_packet,
                   sizeof(size_packet),
                   0,
                   (sockaddr *)&client_addr,
                   sizeof(client_addr));
    if (n == -1) {
        spdlog::error("Failed to send size packet.");
    }
    // send packet next
    n = sendto(sock,
               packet,
               size_packet.size,
               0,
               (sockaddr *)&client_addr,
               sizeof(client_addr));
    if (n == -1) {
        spdlog::error("Failed to send packet.");
        return false;
    }
    return true;
}
