#ifndef HIDO_CLIENT_CLIENT_HPP
#define HIDO_CLIENT_CLIENT_HPP

#include <netinet/in.h>
#include <raylib.h>

#include <atomic>
#include <mutex>
#include <string>

#include "map/map.hpp"
#include "network.hpp"
#include "state/player.hpp"
#include "state_buffer.hpp"

class Client {
  public:
    Client(const std::string &addr, uint32_t port, const std::string &name);
    ~Client();
    void run();

  private:
    void render_state(uint64_t render_time);
    void render_bullets(uint64_t render_time);

    void listen_thread();
    void send_connect_packet();
    void send_disconnect_packet();
    void send_input_packet(const InputPacket &input);
    InputPacket get_input();

    int sock = 0;
    sockaddr_in serv_addr{};
    std::atomic<bool> running = true, connecting = true;
    std::string name;

    constexpr static int WIDTH = 1080, HEIGHT = 720;

    int client_id = -1;
    StateBuffer<GameStatePacket> game_state_buffer;
    StateBuffer<BulletStatePacket> bullet_state_buffer;

    std::mutex state_mutex;

    // textures
    Texture player_texture, bullet_texture, health_bar_texture;
    Camera2D camera;

    PlayerState local_player;
    std::vector<InputPacket> unacknowledged; // increasing order of timestamps
    std::unique_ptr<GameMap> map;
};

#endif // HIDO_CLIENT_CLIENT_HPP
