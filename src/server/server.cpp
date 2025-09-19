#include "server.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <raymath.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>

#include "network.hpp"
#include "server/client_manager.hpp"
#include "state/bullet.hpp"
#include "state/player.hpp"

Server::Server(uint32_t port) {
    // SOCK_DGRAM: Datagram sockets are for UDP (different order/duplicate msgs)
    // SOCK_STREAM: TCP sequenced, constant, 2 way stream of data
    // SOCK_RAW: ICMP, not used
    // SOCK_SEQPACKET: record boundaries preserved
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        spdlog::error("Error creating connection.");
        running = false;
        return;
    }
    spdlog::info("Successfully created socket.");

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    // htons: correct endian ordering with bits
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0) {
        spdlog::error("Error binding socket.");
        running = false;
        return;
    }
    spdlog::info("Successfully binded socket.");
    epfd = epoll_create1(0);
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        spdlog::error("Error epoll ctl.");
        running = false;
        return;
    }

    spdlog::info("Listening on port {}.", port);
}

Server::~Server() {
    close(epfd);
    close(sock);
}

void Server::serve() {
    const size_t MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    auto last_t = std::chrono::steady_clock::now();

    const auto tick_interval = std::chrono::milliseconds(TICK_INTERVAL);

    map = std::make_unique<GameMap>("./res/map/map1.tmx", "./res/map");

    while (running) {
        // wait time of 0 causes return immediately
        // TODO: wait a little to not hog CPU
        int n_ready = epoll_wait(epfd, events, MAX_EVENTS, 10);
        if (n_ready <= 0) continue;

        for (int i = 0; i < n_ready; ++i) {
            if (events[i].events & EPOLLIN) {
                process_events();
            }
        }

        // update the game
        auto now = std::chrono::steady_clock::now();
        if (now - last_t >= tick_interval) {
            double dt = std::chrono::duration<double>(now - last_t).count();
            last_t = now;
            update(dt);

            uint64_t timestamp =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch())
                    .count();
            send_game_state(timestamp);
            send_bullet_state(timestamp);
        }
    }
}

void Server::shutdown() {
    spdlog::info("Shutting down.");
    running = false;
}

void Server::process_events() {
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);

    Packet packet;
    // get packet
    int n = recvfrom(
        sock, packet.data(), packet.size(), 0, (sockaddr *)&client_addr, &len);
    // WARN: only since UDP sends entire packets, we can assume everything
    // arrived
    if (n <= 0) {
        return;
    }
    // get header
    PacketHeader *header = get_header(packet);
    std::vector<BulletPacket> bullet_packets;

    // set the client/sender id
    if (manager.count() >= MAX_PLAYERS) {
        spdlog::warn("Game server already hosting max of {} players.",
                     MAX_PLAYERS);
        return;
    }
    // add or return if client already exists
    ClientAddr &c = manager.add(client_addr);
    header->sender = c.id;

    if (header->type == PacketType::INPUT) {
        // update last input packet for the corresponding client
        InputPacket *input_packet = get_packet_data<InputPacket>(packet);
        if (input_packet->header.timestamp > c.last_input.header.timestamp) {
            c.last_input = *input_packet;
            c.player.id = c.id;
        }
    }

    // DISCONNECT PACKET
    else if (header->type == PacketType::CLIENT_DISCONNECT) {
        // TODO: client keeps sending disconnect until server acknowledges
        manager.remove(c);
    }
}

void Server::update(float dt) {
    // update clients
    for (auto &client : manager.get_clients()) {
        auto &player = client.second.player;
        auto &input = client.second.last_input;
        Vector2 vel{(input.right - input.left) * PLAYER_SPEED,
                    (input.down - input.up) * PLAYER_SPEED};
        player_update(player, vel, dt, *map);
        // bullets
        if (input.mouse_down) {
            Vector2 direction = Vector2Subtract(input.mouse_pos,
                                                {player.rect.x, player.rect.y});
            direction = Vector2Normalize(direction);
            Vector2 vel = Vector2Scale(direction, 300.0f);
            uint64_t t = get_now_millis();
            bullet_state.push_back(
                BulletState{t,
                            player.id,
                            bullet_idx++,
                            Vector2{player.rect.x, player.rect.y},
                            vel});
        }
    }
    // move bullets
    for (auto itr = bullet_state.begin(); itr != bullet_state.end(); ++itr) {
        bool wall_collision = bullet_update(*itr, dt, *map);
        if (wall_collision) {
            // smart erase is marginally faster than classic erase despite
            // memmove optimizations
            std::swap(*itr, bullet_state.back());
            bullet_state.pop_back();
            itr--;
        }
    }
    // check if bullets hit any clients
    for (auto itr = bullet_state.begin(); itr != bullet_state.end(); ++itr) {
        for (auto &client : manager.get_clients()) {
            // check if client and bullet have reasonable timestamp similarity
            uint64_t a = itr->timestamp,
                     b = client.second.last_input.header.timestamp;
            uint64_t diff = a > b ? a - b : b - a;
            // if they're within a frame
            if (std::abs((int64_t)diff) > TICK_INTERVAL) {
                continue;
            }

            auto &player = client.second.player;
            // only non-player's bullets can hurt
            if (player.id == itr->sender) continue;
            if (CheckCollisionRecs(
                    player.rect,
                    {itr->pos.x, itr->pos.y, BULLET_SIZE, BULLET_SIZE})) {
                // smart erase is marginally faster than classic erase despite
                // memmove optimizations
                std::swap(*itr, bullet_state.back());
                bullet_state.pop_back();
                itr--;
                player.health -= 0.2f;
            }
        }
    }
}

void Server::send_game_state(uint64_t timestamp) {
    // send clients the updates
    GameStatePacket gsp;
    gsp.header.type = PacketType::GAME_STATE;
    gsp.header.timestamp = timestamp;
    gsp.num_players = manager.count();

    // add them in sorted order
    size_t i = 0;
    for (auto &client : manager.get_clients()) {
        gsp.players[i++] = client.second.player;
    }

    // create the packet and send it
    Packet packet = create_packet_from<GameStatePacket>(gsp);
    for (auto &client : manager.get_clients()) {
        // update the client_id to tell the client what their id is
        gsp.client_id = client.first;
        size_t offset = offsetof(GameStatePacket, client_id);
        memcpy(packet.data() + offset, &client.first, sizeof(gsp.client_id));

        // send the packet
        sendto(sock,
               packet.data(),
               sizeof(GameStatePacket),
               0,
               (sockaddr *)&client.second.addr,
               sizeof(client.second.addr));
    }
}

void Server::send_bullet_state(uint64_t timestamp) {
    BulletStatePacket bsp;
    bsp.header.type = PacketType::BULLET;
    bsp.header.timestamp = timestamp;
    bsp.num_bullets = std::min(MAX_BULLETS_PER_PACKET, bullet_state.size());
    // TODO: alternate sending bullets every frame for more space
    // add bullet packets
    for (size_t i = 0; i < bsp.num_bullets; ++i) {
        // copy sender, id and pos
        bsp.bullets[i].sender = bullet_state[i].sender;
        bsp.bullets[i].id = bullet_state[i].id;
        bsp.bullets[i].pos = bullet_state[i].pos;
    }
    Packet packet = create_packet_from<BulletStatePacket>(bsp);
    // send packet to clients
    for (auto &client : manager.get_clients()) {
        // send the packet
        sendto(sock,
               packet.data(),
               sizeof(bsp.header) + sizeof(bsp.num_bullets) +
                   sizeof(BulletPacket) * bsp.num_bullets,
               0,
               (sockaddr *)&client.second.addr,
               sizeof(client.second.addr));
    }
}
