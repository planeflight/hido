#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <thread>

#include "bullet.hpp"
#include "map/map.hpp"
#include "map/map_renderer.hpp"
#include "network.hpp"
#include "util.hpp"

Client::Client(int port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        spdlog::error("Error opening client connection.");
        return;
    }
    spdlog::info("Successfully established client connection.");
}

Client::~Client() {
    CloseWindow();
    close(sock);
}

void Client::run() {
    spdlog::info("Running client.");
    std::thread listening{[&]() { listen_thread(); }};

    SetTargetFPS(60);
    InitWindow(WIDTH, HEIGHT, "HIDO");

    player = std::make_unique<Player>(Rectangle{20.0f, 20.0f, 8.0f, 12.0f});
    Texture player_texture = LoadTexture("./res/player/idle.png");
    Texture bullet_texture = LoadTexture("./res/bullet.png");
    player->texture = player_texture;

    Camera2D camera;
    camera.zoom = 3.0f;
    camera.offset = {WIDTH / 2.0f, HEIGHT / 2.0f};
    camera.target = {player->rect.x + player->rect.width / 2.0f,
                     player->rect.y + player->rect.height / 2.0f};
    camera.rotation = 0.0f;

    GameMap map("./res/map/map1.tmx", "./res/map");
    MapRenderer map_renderer(&map, "./res/map");

    float last_t = GetTime();
    while (!WindowShouldClose()) {
        float dt = (GetTime() - last_t);
        last_t = GetTime();

        // input
        player->input(dt);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            bullets.push_back(
                Bullet(Rectangle{player->rect.x, player->rect.y, 6.0f, 6.0f},
                       Vector2{(float)sign(player->vel.x),
                               (float)sign(player->vel.y)}));
        }

        // update
        player->update(dt, map);
        for (int i = bullets.size() - 1; i >= 0; --i) {
            bullets[i].update(dt, map, {});
            if (bullets[i].get_destroy()) {
                bullets.erase(bullets.begin() + i);
            }
        }

        camera.target.x +=
            (player->rect.x + player->rect.width / 2.0f - camera.target.x) *
            0.05f;
        camera.target.y +=
            (player->rect.y + player->rect.height / 2.0f - camera.target.y) *
            0.05f;

        // network updates
        send_update_packet();
        // send_bullet_packet();

        // render
        BeginDrawing();
        ClearBackground(Color{9, 10, 20, 255});
        BeginMode2D(camera);

        map_renderer.render();
        player->render();

        for (Bullet &b : bullets) {
            b.render(bullet_texture);
        }
        // draw other bullets
        bullets_mutex.lock();
        for (const auto &enemy_bullets : latest_enemy_bullet) {
            for (const BulletPacket &packet : enemy_bullets.second) {
                Bullet b(Rectangle{packet.x, packet.y, 6.0f, 6.0f}, {});
                b.render(bullet_texture);
            }
        }
        bullets_mutex.unlock();

        // draw other clients
        clients_mutex.lock();
        for (const auto &client_packet : clients) {
            Player c{Rectangle{client_packet.x, client_packet.y, 8.0f, 12.0f}};
            c.texture = player_texture;
            c.render();
        }
        clients_mutex.unlock();

        EndMode2D();
        EndDrawing();
    }
    // broadcast disconnect
    send_disconnect_packet();

    UnloadTexture(player_texture);
    UnloadTexture(bullet_texture);
    running = false;
    if (listening.joinable()) listening.join();

    spdlog::info("Client shutting down.");
}

void Client::listen_thread() {
    SizePacket size_packet;
    ClientPacket packet;
    pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    while (running) {
        // non-blocking allows disconnect
        if (poll(fds, 1, 10) < 0) {
            spdlog::error("Polling error.");
            continue;
        }
        if (fds[0].revents & POLLIN) {
            while (running) {
                sockaddr_in from{};
                socklen_t len = sizeof(from);
                int n = recvfrom(sock,
                                 &size_packet,
                                 sizeof(size_packet),
                                 MSG_DONTWAIT,
                                 (sockaddr *)&from,
                                 &len);
                if (n <= 0) {
                    // spdlog::error("Failed to receive size packet.");
                    break;
                }
                if (size_packet.type == SizePacket::IncomingType::CLIENT) {
                    n = recvfrom(sock,
                                 &packet,
                                 sizeof(packet),
                                 MSG_DONTWAIT,
                                 (sockaddr *)&from,
                                 &len);

                    if (n <= 0) {
                        break;
                    }
                    auto client_itr =
                        std::find_if(clients.begin(),
                                     clients.end(),
                                     [&packet](const ClientPacket &p) {
                                         return p.idx == packet.idx;
                                     });
                    clients_mutex.lock();
                    // update packet if found and timestamp is updated
                    if (client_itr != clients.end()) {
                        if (packet.header.type == PacketType::DISCONNECT) {
                            spdlog::info("Player {} disconnected.", packet.idx);
                            // remove all of this client's bullets as well
                            latest_enemy_bullet[packet.idx] = {};
                            clients.erase(client_itr);
                        }
                        // otherwise just update the packet
                        else if (packet.header.timestamp >
                                 client_itr->header.timestamp) {
                            *client_itr = packet;
                        }
                    }
                    // otherwise add new packet
                    else {
                        spdlog::info("New player connected. id: {}",
                                     packet.idx);
                        clients.push_back(packet);
                    }
                    clients_mutex.unlock();
                }

                // bullet packet
                else if (size_packet.type == SizePacket::IncomingType::BULLET) {
                    fmt::println("received bullet packet.");
                    std::vector<BulletPacket> enemy_bullets;
                    enemy_bullets.resize(size_packet.size /
                                         sizeof(BulletPacket));
                    n = recvfrom(sock,
                                 enemy_bullets.data(),
                                 size_packet.size,
                                 0,
                                 (sockaddr *)&from,
                                 &len);
                    bullets_mutex.lock();
                    // TODO: use timestamps to update
                    latest_enemy_bullet[size_packet.sender] = enemy_bullets;
                    bullets_mutex.unlock();
                }
            }
        }
    }
}

void Client::send_client_packet(ClientPacket &packet) {
    // send size packet
    SizePacket size_packet{.timestamp = ++timestamp,
                           .size = sizeof(packet),
                           .type = SizePacket::IncomingType::CLIENT};

    sendto(sock,
           &size_packet,
           sizeof(size_packet),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));

    // send actual packet
    sendto(sock,
           &packet,
           sizeof(packet),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}

void Client::send_update_packet() {
    ClientPacket packet{
        .header = {.timestamp = timestamp, .type = PacketType::UPDATE},
        .x = player->rect.x,
        .y = player->rect.y};
    send_client_packet(packet);
}

void Client::send_disconnect_packet() {
    ClientPacket packet{
        .header = {.timestamp = timestamp, .type = PacketType::DISCONNECT}
        // don't bother with the rest
    };
    send_client_packet(packet);
}

void Client::send_bullet_packet() {
    // don't send if there are 0 bullets
    if (bullets.size() == 0) return;

    const size_t n = bullets.size();
    // send size packet
    SizePacket size_packet{.timestamp = ++timestamp,
                           .size = sizeof(BulletPacket) * n,
                           .type = SizePacket::IncomingType::BULLET};
    sendto(sock,
           &size_packet,
           sizeof(size_packet),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));

    // construct actual packet
    std::vector<BulletPacket> bpackets;
    bpackets.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const Vector2 &pos = bullets[i].get_pos();
        bpackets[i] = BulletPacket{.x = pos.x, .y = pos.y};
    }
    // send actual packet
    sendto(sock,
           bpackets.data(),
           size_packet.size,
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}
