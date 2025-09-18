#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>

#include "map/map.hpp"
#include "server/client_manager.hpp"
#include "state/bullet.hpp"

class Server {
  public:
    Server(uint32_t port);
    ~Server();

    void client_accept();
    void serve();
    void shutdown();

  private:
    void process_events();
    void update(float dt);
    void send_game_state(uint64_t timestamp);
    void send_bullet_state(uint64_t timestamp);

    int sock = 0;
    int epfd = 0;
    std::atomic<bool> running = true;

    // world objects
    std::unique_ptr<GameMap> map = nullptr;

    ClientManager manager;
    std::vector<BulletState> bullet_state;
    int bullet_idx = 0;
};

#endif // SERVER_HPP
