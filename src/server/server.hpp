#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <poll.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

class Server {
  public:
    Server(uint32_t port);
    ~Server();

    void client_accept();
    void serve();
    void shutdown();

  private:
    void process_events();

    int sock = 0;
    int epfd = 0;
    std::atomic<bool> running = true;
    std::vector<pollfd> fds;

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

    std::vector<Client> clients;
};

#endif // SERVER_HPP
