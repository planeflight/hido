#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "car.hpp"
#include "network.hpp"

class Client {
  public:
    Client(int port);
    ~Client();
    void run();

  private:
    void listen_thread();
    void send_update_packet();
    void send_disconnect_packet();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true;

    std::unique_ptr<Car> player = nullptr;

    constexpr static int WIDTH = 1280, HEIGHT = 720;

    // track others: latest client update packet
    std::vector<UpdatePacket> clients;
    std::mutex mutex;
    uint32_t timestamp = 0;
};

#endif // CLIENT_HPP
