#include "server.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <set>
#include <thread>
#include <vector>

#include "network.hpp"
#include "server/client_manager.hpp"
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

    const auto tick_interval = std::chrono::milliseconds((uint32_t)1000 / 60);

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
    // A BULLET COLLISION PACKET
    else if (header->type == PacketType::BULLET_COLLISION) {
        BulletCollisionPacket *bcp =
            get_packet_data<BulletCollisionPacket>(packet);

        // find receiver of the bullet collision's address
        ClientAddr receiver_addr = manager.find_by_id(bcp->to);

        // send message to the client in particular
        sendto(sock,
               packet.data(),
               packet.size(),
               0,
               (sockaddr *)&receiver_addr.addr,
               sizeof(receiver_addr.addr));
        return;
    }
}

void Server::update(float dt) {
    for (auto &client : manager.get_clients()) {
        auto &player = client.second.player;
        auto &input = client.second.last_input;
        Vector2 vel{(input.right - input.left) * PLAYER_SPEED,
                    (input.down - input.up) * PLAYER_SPEED};
        player_update(player, vel, dt, *map);
    }
    // if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    //     Vector2 direction,
    //         mouse_pos = GetMousePosition(),
    //         world_pos = GetScreenToWorld2D(mouse_pos, camera);
    //     direction.x = world_pos.x - player->rect.x;
    //     direction.y = world_pos.y - player->rect.y;
    //     direction = Vector2Normalize(direction);
    //
    //     bullets.push_back(
    //         Bullet(Rectangle{player->rect.x, player->rect.y, 6.0f, 6.0f},
    //                direction));
    // }
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
               packet.size(),
               0,
               (sockaddr *)&client.second.addr,
               sizeof(client.second.addr));
    }
}
