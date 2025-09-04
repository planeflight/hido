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
    };

    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];
    // TODO: change to input buffer
    char buffer[1024];
    std::set<Client> clients;

    while (running) {
        // wait time of 0 causes return immediately
        int n_ready = epoll_wait(epfd, events, MAX_EVENTS, 0);
        if (n_ready <= 0) continue;

        for (int i = 0; i < n_ready; ++i) {
            if (events[i].events & EPOLLIN) {
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                int n = recvfrom(sock,
                                 buffer,
                                 sizeof(buffer),
                                 0,
                                 (sockaddr *)&client_addr,
                                 &len);
                // TODO: disconnect messages
                if (n > 0) {
                    buffer[n] = '\0';
                    std::string msg(buffer);
                    Client c{client_addr};
                    clients.insert(c);
                    fmt::println("Received from client: {}", msg);

                    // broadcast to clients
                    std::string response = "Echo: " + msg;
                    for (auto &cl : clients) {
                        sendto(sock,
                               response.c_str(),
                               response.size(),
                               0,
                               (sockaddr *)&cl.addr,
                               sizeof(cl.addr));
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
