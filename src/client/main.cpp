#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

#include "client.hpp"
#include "network.hpp"

int main(int argc, char **argv) {
    if (argc != 4) {
        spdlog::error("Invalid usage: ./client <address> <port> <name>");
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
    std::string name = argv[3];
    if (name.length() > MAX_NAME_LENGTH) {
        spdlog::error("Invalid name: Must be <= {} characters.",
                      MAX_NAME_LENGTH);
        return -1;
    }
    Client client(argv[1], port, name);
    client.run();
    return 0;
}
