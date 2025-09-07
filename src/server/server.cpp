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
#include "spdlog/fmt/bundled/base.h"

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
    struct Client {
        sockaddr_in addr;
        bool operator<(const Client &other) const {
            return memcmp(&addr, &other.addr, sizeof(sockaddr_in)) < 0;
        }

        bool operator==(const Client &other) const {
            return addr.sin_addr.s_addr == other.addr.sin_addr.s_addr &&
                   addr.sin_port == other.addr.sin_port;
        }
    };

    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];
    // TODO: change to input buffer
    UpdatePacket packet;
    std::vector<Client> clients;

    while (running) {
        // wait time of 0 causes return immediately
        // TODO: wait a little to not hog CPU
        int n_ready = epoll_wait(epfd, events, MAX_EVENTS, 10);
        if (n_ready <= 0) continue;

        for (int i = 0; i < n_ready; ++i) {
            if (events[i].events & EPOLLIN) {
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                int n = recvfrom(sock,
                                 &packet,
                                 sizeof(packet),
                                 0,
                                 (sockaddr *)&client_addr,
                                 &len);
                // TODO: disconnect messages
                if (n > 0) {
                    Client c{client_addr};
                    auto client_itr =
                        std::find(clients.begin(), clients.end(), c);
                    // get id
                    size_t idx = std::distance(clients.begin(), client_itr);
                    if (client_itr == clients.end()) {
                        clients.push_back(c);
                        idx = clients.size() - 1;
                    }
                    packet.idx = idx;
                    // check if disconnect
                    if (packet.header.type == PacketType::DISCONNECT) {
                        clients.erase(clients.begin() + idx);
                    }
                    // broadcast message to clients regardless
                    for (auto &cl : clients) {
                        // send to all different clients
                        if (cl != c) {
                            sendto(sock,
                                   &packet,
                                   sizeof(packet),
                                   0,
                                   (sockaddr *)&cl.addr,
                                   sizeof(cl.addr));
                        }
                    }
                }
            }
        }
    }
}

void Server::shutdown() {
    spdlog::info("Shutting down.");
    running = false;
}
