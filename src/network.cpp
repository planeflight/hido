#include "network.hpp"

#include <spdlog/spdlog.h>
#include <sys/socket.h>

#include <cstring>

Packet create_client_packet(ClientPacket packet) {
    Packet p;
    auto dest = p.data();
    memcpy(dest, &packet, sizeof(packet));
    return p;
}

PacketHeader *get_header(Packet &packet) {
    return (PacketHeader *)packet.data();
}

ClientPacket *get_client_packet_data(Packet &packet) {
    return (ClientPacket *)&packet;
}

Packet create_bullet_packet(PacketHeader building_header,
                            const std::vector<BulletPacket> &bullets) {
    Packet packet;
    int8_t *data_start = packet.data() + sizeof(building_header);
    // copy bullets into packet
    memcpy(data_start, bullets.data(), bullets.size() * sizeof(BulletPacket));

    building_header.size = bullets.size() * sizeof(BulletPacket);
    // copy header
    memcpy(&packet, &building_header, sizeof(building_header));
    return packet;
}

void get_bullet_data(Packet &packet, std::vector<BulletPacket> &bullets) {
    // TODO: error handling with wrong types
    const size_t n = get_header(packet)->size / sizeof(BulletPacket);
    bullets.resize(n);
    memcpy(bullets.data(),
           packet.data() + sizeof(PacketHeader),
           get_header(packet)->size);
}
