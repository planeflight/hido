#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <netinet/in.h>
#include <raylib.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "bullet.hpp"
#include "network.hpp"

class Client {
  public:
    Client(int port);
    ~Client();
    void run();

  private:
    void render_state(uint64_t render_time);

    void listen_thread();
    void send_client_packet(const Packet &packet);
    void send_update_packet();
    void send_disconnect_packet();
    void send_bullet_packet();
    void send_bullet_collision_packet(int client_id);
    void send_input_packet();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true;

    constexpr static int WIDTH = 720, HEIGHT = 720;

    std::deque<GameStatePacket> state_buffer;

    // this player's bullets
    std::vector<Bullet> bullets;
    // any other enemy bullets <id, <timestamp, bullet_list>>
    std::unordered_map<int, std::pair<uint32_t, std::vector<BulletPacket>>>
        latest_enemy_bullet;
    std::mutex bullets_mutex;

    std::mutex state_mutex;
    uint32_t timestamp = 0;

    // textures
    Texture player_texture, bullet_texture, health_bar_texture;
    Camera2D camera;
};

#endif // CLIENT_HPP
