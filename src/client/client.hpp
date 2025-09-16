#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
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
    void send_client_packet(const Packet &packet);
    void send_update_packet();
    void send_disconnect_packet();
    void send_bullet_packet();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true;

    std::unique_ptr<Player> player = nullptr;

    constexpr static int WIDTH = 1080, HEIGHT = 1080;

    // this player's bullets
    std::vector<Bullet> bullets;
    // any other enemy bullets
    std::unordered_map<int, std::vector<BulletPacket>> latest_enemy_bullet;
    std::mutex bullets_mutex;

    // track others: latest client update packet
    std::vector<Packet> clients;
    std::mutex clients_mutex;
    uint32_t timestamp = 0;
};

#endif // CLIENT_HPP
