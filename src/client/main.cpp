#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

#include "client.hpp"

int main(int argc, char **argv) {
    if (argc != 3) {
        spdlog::error("Invalid usage: ./client <address> <port>");
        return -1;
    }
    int port = 8080;
    try {
        port = std::stoi(argv[2]);
    } catch (std::invalid_argument const &e) {
        spdlog::error("std::invalid argument: {}", e.what());
        return -1;
    } catch (std::out_of_range const &e) {
        spdlog::error("std::out_of_range: {}", e.what());
        return -1;
    }
    Client client(argv[1], port);
    client.run();
    return 0;
}
