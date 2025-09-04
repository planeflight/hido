#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>

#include <atomic>

class Client {
  public:
    Client(int port);
    ~Client();
    void run();

  private:
    void listen_thread();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true;
};

#endif // CLIENT_HPP
