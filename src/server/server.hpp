#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>

#include "map/map.hpp"
#include "network.hpp"
#include "server/client_manager.hpp"

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

    int sock = 0;
    int epfd = 0;
    std::atomic<bool> running = true;

    ClientManager manager;

    std::set<Packet> input_packets;

    // world objects
    std::unique_ptr<GameMap> map = nullptr;
};

#endif // SERVER_HPP
