#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

Client::Client(int port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        spdlog::error("Error opening client connection.");
        return;
    }
    spdlog::info("Successfully established client connection.");
}

Client::~Client() {
    close(sock);
}

void Client::run() {
    spdlog::info("Running client.");
    std::thread listening{[&]() { listen_thread(); }};

    std::string msg;

    while (running) {
        std::getline(std::cin, msg);
        sendto(sock,
               msg.c_str(),
               msg.size(),
               0,
               (sockaddr *)&serv_addr,
               sizeof(serv_addr));
    }

    listening.join();
    spdlog::info("Client shutting down.");
}

void Client::listen_thread() {
    char buffer[1024];
    while (running) {
        sockaddr_in from{};
        socklen_t len = sizeof(from);
        int n =
            recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr *)&from, &len);
        if (n > 0) {
            buffer[n] = '\0';
            fmt::println("Server: {}", buffer);
        }
    }
}
