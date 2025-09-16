#include "server.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <cstdint>
#include <set>
#include <thread>
#include <vector>

#include "network.hpp"
#include "server/client_manager.hpp"

Server::Server(uint32_t port) {
    // SOCK_DGRAM: Datagram sockets are for UDP (different order/duplicate msgs)
    // SOCK_STREAM: TCP sequenced, constant, 2 way stream of data
    // SOCK_RAW: ICMP, not used
    // SOCK_SEQPACKET: record boundaries preserved
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        spdlog::error("Error creating connection.");
        running = false;
        return;
    }
    spdlog::info("Successfully created socket.");

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    // htons: correct endian ordering with bits
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0) {
        spdlog::error("Error binding socket.");
        running = false;
        return;
    }
    spdlog::info("Successfully binded socket.");
    epfd = epoll_create1(0);
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        spdlog::error("Error epoll ctl.");
        running = false;
        return;
    }

    // if (listen(sock, 1) < 0) {
    //     spdlog::error("Error listening to socket.");
    //     running = false;
    //     return;
    // }
    spdlog::info("Listening on port {}.", port);
}

Server::~Server() {
    for (pollfd &fd : fds) {
        close(fd.fd);
    }
    close(epfd);
    close(sock);
}

void Server::serve() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (running) {
        // wait time of 0 causes return immediately
        // TODO: wait a little to not hog CPU
        int n_ready = epoll_wait(epfd, events, MAX_EVENTS, 10);
        if (n_ready <= 0) continue;

        for (int i = 0; i < n_ready; ++i) {
            if (events[i].events & EPOLLIN) {
                process_events();
            }
        }
    }
}

void Server::shutdown() {
    spdlog::info("Shutting down.");
    running = false;
}

void Server::process_events() {
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);

    Packet packet;
    // get packet
    int n = recvfrom(
        sock, packet.data(), packet.size(), 0, (sockaddr *)&client_addr, &len);
    // WARN: only since UDP sends entire packets, we can assume everything
    // arrived
    if (n <= 0) {
        return;
    }
    // get header
    PacketHeader *header = get_header(packet);
    std::vector<BulletPacket> bullet_packets;

    // set the client/sender id
    const ClientAddr &c = manager.add(client_addr);
    header->sender = c.id;

    // A CLIENT UPDATE
    if (header->type == PacketType::CLIENT_UPDATE ||
        header->type == PacketType::CLIENT_DISCONNECT) {
        // check if disconnect
        if (header->type == PacketType::CLIENT_DISCONNECT) {
            spdlog::info("Client {} disconnected.", c.id);
            manager.remove(c);
        }
    }
    // A BULLET PACKET
    else if (header->type == PacketType::BULLET) {}
    // broadcast message to all other clients
    for (const auto &cl : manager.get_clients()) {
        if (cl != c) {
            sendto(sock,
                   packet.data(),
                   packet.size(),
                   0,
                   (sockaddr *)&cl.addr,
                   sizeof(cl.addr));
        }
    }
}
