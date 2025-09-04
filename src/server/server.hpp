#ifndef SERVER_HPP
#define SERVER_HPP

#include <poll.h>

#include <atomic>
#include <cstdint>
#include <vector>

class Server {
  public:
    Server(uint32_t port);
    ~Server();

    void client_accept();
    void serve();
    void shutdown();

  private:
    bool check_disconnect(int client, size_t size);

    int sock = 0;
    std::atomic<bool> running = true;
    std::vector<pollfd> fds;
};

#endif // SERVER_HPP
