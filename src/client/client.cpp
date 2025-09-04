#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <vector>

#include "network.hpp"

std::string load_file(const std::string &file_name) {
    std::ifstream t(file_name);
    std::stringstream buffer;
    buffer << t.rdbuf();
    std::string source = buffer.str();
    return source;
}

Client::Client(int port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        spdlog::error("Error opening client connection.");
        return;
    }
    spdlog::info("Successfully established client connection.");
}

void Client::run() {
    spdlog::info("Running client.");
    pollfd client_poll;
    client_poll.fd = sock;
    client_poll.events = POLLIN;

    int size;
    uint32_t size_read = 0;
    Message buffer_msg{.bytes = 0};

    enum class State { SIZE, BUFFER } state = State::SIZE;

    while (true) {
        if (poll(&client_poll, 1, 33) < 0) {
            spdlog::error("Poll error.");
            continue;
        }

        if (client_poll.revents & POLLIN) {
            if (state == State::SIZE) {
                char *offset = ((char *)&size) + size_read;
                int bytes = recv(sock, offset, sizeof(size) - size_read, 0);
                if (bytes == 0) {
                    spdlog::info("Server shut down.");
                    break;
                } else if (bytes < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    spdlog::error("Unexpected error.");
                    break;
                }
                // increment number of bytes read
                size_read += bytes;
                // check if we read sizeof(int) = 4 bytes
                if (size_read == sizeof(size)) {
                    size = ntohl(size);
                    state = State::BUFFER;
                    buffer_msg.buffer.resize(size);
                    buffer_msg.bytes = 0;
                }
            } else {
                unsigned char *offset =
                    buffer_msg.buffer.data() + buffer_msg.bytes;
                int bytes = recv(sock, offset, size - buffer_msg.bytes, 0);
                if (bytes == 0) {
                    spdlog::info("Server shut down.");
                    break;
                } else if (bytes < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    spdlog::error("Unexpected error.");
                    break;
                }
                buffer_msg.bytes += bytes;
                if (buffer_msg.bytes == (size_t)size) {
                    state = State::SIZE;
                    size_read = 0;
                }
            }
        }
    }
    spdlog::info("Client shutting down.");
    close(sock);
}
