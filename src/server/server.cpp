#include "server.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <cstdint>
#include <thread>
#include <vector>

#include "network.hpp"

Server::Server(uint32_t port) {
    // SOCK_DGRAM: Datagram sockets are for UDP (different order/duplicate msgs)
    // SOCK_STREAM: TCP sequenced, constant, 2 way stream of data
    // SOCK_RAW: ICMP, not used
    // SOCK_SEQPACKET: record boundaries preserved
    sock = socket(AF_INET, SOCK_STREAM, 0);
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
    if (listen(sock, 1) < 0) {
        spdlog::error("Error listening to socket.");
        running = false;
        return;
    }
    spdlog::info("Listening on port {}.", port);
}

Server::~Server() {
    for (pollfd &fd : fds) {
        close(fd.fd);
    }
    close(sock);
}

void Server::client_accept() {
    // POLLIN	Alert me when data is ready to recv() on this socket.
    // POLLOUT	Alert me when I can send() data to this socket without blocking.
    // POLLHUP	Alert me when the remote closed the connection.

    // add our server socket
    pollfd server_poll;
    server_poll.fd = sock;
    server_poll.events = POLLIN;
    fds.push_back(server_poll);

    // forces this thread to join later
    while (running) {
        // -1 blocks until event occurs
        // no-block with ~100 ms in case server closes
        if (poll(fds.data(), fds.size(), 100) < 0) {
            spdlog::error("Poll error.");
            break;
        }
        for (size_t i = 0; i < fds.size(); ++i) {
            // if there's data to read/recv
            if (fds[i].revents & POLLIN) {
                // if server socket has data, there's connect/disconnect
                if (fds[i].fd == sock) {
                    sockaddr_in client;
                    socklen_t client_len = sizeof(client);
                    int client_fd =
                        accept(sock, (sockaddr *)&client, &client_len);
                    if (client_fd < 0) {
                        spdlog::error(
                            "Error establishing connection to client.");
                        continue;
                    }
                    spdlog::info("Established connection with client {}.",
                                 client_fd);
                    pollfd client_poll;
                    client_poll.fd = client_fd;
                    client_poll.events = POLLIN;
                    fds.push_back(client_poll);
                }
                // otherwise client is giving info
                else {
                    char *buffer;
                    size_t BUF_SIZE = 1024;
                    size_t n = recv(fds[i].fd, buffer, BUF_SIZE, 0);
                    if (n <= 0) {
                        spdlog::info("Disconnected client {}.", fds[i].fd);
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        i--; // adjust index
                    }
                }
            }
        }
    }
}

void Server::serve() {
    if (!running) return;

    std::thread accept_thread([&]() { client_accept(); });

    std::vector<unsigned char> buffer = {'h', 'i', '\0'};
    while (running) {
        int size = buffer.size();
        size = htonl(size);
        for (int i = 1; i < fds.size(); ++i) {
            send(fds[i].fd, &size, sizeof(size), 0);          // send size first
            send(fds[i].fd, buffer.data(), buffer.size(), 0); // then data
        }
    }
    if (accept_thread.joinable()) {
        accept_thread.join();
    }
}

void Server::shutdown() {
    spdlog::info("Shutting down.");
    running = false;
}

bool Server::check_disconnect(int client, size_t size) {
    char buf;
    // MSG_PEEK treats data as unread
    ssize_t bytes_received = recv(client, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    // socket is closed by the peer
    if (bytes_received == 0) {
        spdlog::info("Disconnected from client.");
        return true;
    }
    // no data available because client isn't sending anything
    // obviously, but socket is still open
    if (bytes_received == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return false;
    }
    // Other error occurred, potentially indicating a closed
    // connection
    if (bytes_received == -1) {
        spdlog::error("Unexpected error occured.");
        return true;
    }
    return true;
}
