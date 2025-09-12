#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "bullet.hpp"
#include "network.hpp"
#include "player.hpp"

class Client {
  public:
    Client(int port);
    ~Client();
    void run();

  private:
    void listen_thread();
    void send_client_packet(ClientPacket &packet);
    void send_update_packet();
    void send_disconnect_packet();
    void send_bullet_packet();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true;

    std::unique_ptr<Player> player = nullptr;

    constexpr static int WIDTH = 1080, HEIGHT = 1080;

    std::vector<Bullet> bullets;
    std::vector<Bullet> enemy_bullets;

    // track others: latest client update packet
    std::vector<ClientPacket> clients;
    std::mutex mutex;
    uint32_t timestamp = 0;
};

#endif // CLIENT_HPP
