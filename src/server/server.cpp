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
    SizePacket size_packet;

    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    // get size packet
    int n = recvfrom(sock,
                     &size_packet,
                     sizeof(size_packet),
                     0,
                     (sockaddr *)&client_addr,
                     &len);
    // WARN: only since UDP sends entire packets, we can assume everything
    // arrived
    if (n <= 0) {
        return;
    }
    // check incoming packet type
    ClientPacket packet;
    Client c{client_addr};
    if (size_packet.type == SizePacket::IncomingType::CLIENT) {
        n = recvfrom(
            sock, &packet, sizeof(packet), 0, (sockaddr *)&client_addr, &len);
        if (n <= 0) {
            return;
        }
        c = Client{client_addr};
        auto client_itr = std::find(clients.begin(), clients.end(), c);
        // get id
        size_t idx = std::distance(clients.begin(), client_itr);
        if (client_itr == clients.end()) {
            clients.push_back(c);
            idx = clients.size() - 1;
            spdlog::info("New client connection {} established.", idx);
        }
        packet.idx = idx;
        // check if disconnect
        if (packet.header.type == PacketType::DISCONNECT) {
            spdlog::info("Client {} disconnected.", idx);
            clients.erase(clients.begin() + idx);
        }
    }
    // broadcast message to clients regardless
    for (auto &cl : clients) {
        // send to all different clients
        if (cl != c) {
            sendto(sock,
                   &size_packet,
                   sizeof(size_packet),
                   0,
                   (sockaddr *)&cl.addr,
                   sizeof(cl.addr));
            sendto(sock,
                   &packet,
                   sizeof(packet),
                   0,
                   (sockaddr *)&cl.addr,
                   sizeof(cl.addr));
        }
    }
}
