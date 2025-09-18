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
    void send_disconnect_packet();
    void send_input_packet();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true;

    constexpr static int WIDTH = 720, HEIGHT = 720;

    std::deque<GameStatePacket> game_state_buffer;
    std::deque<BulletStatePacket> bullet_state_buffer;

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
