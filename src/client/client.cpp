#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <raylib.h>
#include <raymath.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <mutex>
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
            Vector2 direction,
                mouse_pos = GetMousePosition(),
                world_pos = GetScreenToWorld2D(mouse_pos, camera);
            direction.x = world_pos.x - player->rect.x;
            direction.y = world_pos.y - player->rect.y;
            direction = Vector2Normalize(direction);

            bullets.push_back(
                Bullet(Rectangle{player->rect.x, player->rect.y, 6.0f, 6.0f},
                       direction));
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
        timestamp++;
        send_update_packet();
        send_bullet_packet();

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
        {
            const std::lock_guard<std::mutex> bullet_lock_guard(bullets_mutex);
            for (const auto &enemy_bullets : latest_enemy_bullet) {
                for (const BulletPacket &packet : enemy_bullets.second.second) {
                    Bullet b(Rectangle{packet.x, packet.y, 6.0f, 6.0f}, {});
                    b.render(bullet_texture);
                }
            }
        }

        // draw other clients
        {
            const std::lock_guard<std::mutex> client_lock_guard(clients_mutex);
            for (auto &client_packet : clients) {
                ClientPacket *client = get_client_packet_data(client_packet);
                Player c{Rectangle{client->x, client->y, 8.0f, 12.0f}};
                c.texture = player_texture;
                c.render();
            }
        }
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
    Packet packet;
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
                if (recvfrom(sock,
                             packet.data(),
                             packet.size(),
                             MSG_DONTWAIT,
                             (sockaddr *)&from,
                             &len) <= 0) {
                    break;
                }
                PacketHeader *header = get_header(packet);
                if (header->type == PacketType::CLIENT_DISCONNECT ||
                    header->type == PacketType::CLIENT_UPDATE) {
                    auto client_itr = std::find_if(
                        clients.begin(), clients.end(), [&header](Packet &p) {
                            return get_header(p)->sender == header->sender;
                        });
                    const std::lock_guard<std::mutex> client_lo_guard(
                        clients_mutex);
                    // update packet if found
                    if (client_itr != clients.end()) {
                        if (header->type == PacketType::CLIENT_DISCONNECT) {
                            spdlog::info("Player {} disconnected.",
                                         header->sender);
                            // remove all of this client's bullets as well
                            latest_enemy_bullet[header->sender] = {};
                            clients.erase(client_itr);
                        }
                        // otherwise just update the packet
                        else if (header->timestamp >
                                 get_header(*client_itr)->timestamp) {
                            *client_itr = packet;
                        }
                    }
                    // otherwise add new packet
                    else {
                        spdlog::info("New player connected. id: {}",
                                     header->sender);
                        clients.push_back(packet);
                    }
                }

                // bullet packet
                else if (header->type == PacketType::BULLET) {
                    // fmt::println("received bullet packet.");
                    std::vector<BulletPacket> enemy_bullets;
                    get_bullet_data(packet, enemy_bullets);
                    const std::lock_guard<std::mutex> bullet_lock(
                        bullets_mutex);

                    auto &last_enemy_bullet =
                        latest_enemy_bullet[header->sender];
                    // update if timestamp is more recent
                    if (header->timestamp > last_enemy_bullet.first) {
                        last_enemy_bullet.first = header->timestamp;
                        last_enemy_bullet.second = enemy_bullets;
                    }
                }
            }
        }
    }
}

void Client::send_client_packet(const Packet &packet) {
    // send actual packet
    sendto(sock,
           packet.data(),
           packet.size(),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}

void Client::send_update_packet() {
    ClientPacket p{
        .header = {.timestamp = timestamp, .type = PacketType::CLIENT_UPDATE},
        .x = player->rect.x,
        .y = player->rect.y};
    send_client_packet(create_client_packet(p));
}

void Client::send_disconnect_packet() {
    ClientPacket p{.header = {.timestamp = timestamp,
                              .type = PacketType::CLIENT_DISCONNECT},
                   .x = 0.0f,
                   .y = 0.0f};
    send_client_packet(create_client_packet(p));
}

void Client::send_bullet_packet() {
    // even send if there are 0 bullets since clients have to know what bullets
    // are present
    const size_t n = bullets.size();

    // construct actual packet
    std::vector<BulletPacket> bpackets;
    bpackets.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const Vector2 &pos = bullets[i].get_pos();
        bpackets[i] = BulletPacket{.x = pos.x, .y = pos.y};
    }
    PacketHeader header;
    header.type = PacketType::BULLET;
    header.timestamp = timestamp; // same frame as the sending of updates

    Packet packet = create_bullet_packet(header, bpackets);
    // send actual packet
    sendto(sock,
           packet.data(),
           packet.size(),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}
