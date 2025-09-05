#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>

#include <atomic>
#include <memory>

#include "car.hpp"

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

    std::unique_ptr<Car> player = nullptr;

    constexpr static int WIDTH = 1280, HEIGHT = 720;
};

#endif // CLIENT_HPP
