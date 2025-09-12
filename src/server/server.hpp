#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <poll.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include "server/client_manager.hpp"

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

    ClientManager manager;
};

#endif // SERVER_HPP
