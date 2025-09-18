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

#include "map/map.hpp"
#include "map/map_renderer.hpp"
#include "network.hpp"
#include "state/bullet.hpp"
#include "state/player.hpp"
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
    UnloadTexture(player_texture);
    UnloadTexture(bullet_texture);
    UnloadTexture(health_bar_texture);

    CloseWindow();
    close(sock);
}

void Client::run() {
    spdlog::info("Running client.");
    std::thread listening{[&]() { listen_thread(); }};

    SetTargetFPS(60);
    InitWindow(WIDTH, HEIGHT, "HIDO");

    player_texture = LoadTexture("./res/player/idle.png");
    bullet_texture = LoadTexture("./res/bullet.png");
    health_bar_texture = LoadTexture("./res/health_bar.png");

    camera.zoom = 3.0f;
    camera.offset = {WIDTH / 2.0f, HEIGHT / 2.0f};
    camera.rotation = 0.0f;
    camera.target = {0.0f, 0.0f};

    GameMap map("./res/map/map1.tmx", "./res/map");
    MapRenderer map_renderer(&map, "./res/map");
    while (!WindowShouldClose()) {
        // get input and send packet
        send_input_packet();

        // update
        // for (int i = bullets.size() - 1; i >= 0; --i) {
        //     bullets[i].update(dt, map, {});
        //     if (bullets[i].get_destroy()) {
        //         bullets.erase(bullets.begin() + i);
        //     }
        // }

        // check if this client's bullets hit any clients
        // for (auto itr = bullets.begin(); itr != bullets.end(); itr++) {
        //     Bullet &bullet = *itr;
        //     Rectangle bullet_rect{
        //         bullet.get_pos().x, bullet.get_pos().y, 6.0f, 6.0f};
        //     std::lock_guard<std::mutex> client_lock_guard(clients_mutex);
        //
        //     for (Packet &packet : clients) {
        //         ClientPacket *c = get_packet_data<ClientPacket>(packet);
        //         Rectangle client_rect{c->x, c->y, 8.0f, 12.0f};
        //         // if collision, send collided message
        //         if (CheckCollisionRecs(client_rect, bullet_rect)) {
        //             // send_bullet_collision_packet(c->header.sender);
        //             bullets.erase(itr);
        //             itr--;
        //             // no need to check the other clients
        //             break;
        //         }
        //     }
        // }

        // network updates
        timestamp++;
        // send_update_packet();
        // send_bullet_packet();

        // render
        BeginDrawing();
        ClearBackground(Color{9, 10, 20, 255});
        BeginMode2D(camera);
        map_renderer.render();

        if (game_state_buffer.size() >= 2) {
            uint64_t render_time = get_render_time();

            render_state(render_time);
        }
        if (bullet_state_buffer.size() >= 2) {
            uint64_t render_time = get_render_time();
            auto &bsp = bullet_state_buffer.back();
            for (int i = 0; i < bsp.num_bullets; i++) {
                Color color = WHITE;
                if (client_id >= 0 && bsp.bullets[i].sender != client_id) {
                    color = Color{50, 20, 235, 255};
                }
                bullet_render(bsp.bullets[i].pos, bullet_texture, color);
            }
        }
        EndMode2D();
        EndDrawing();
    }
    // broadcast disconnect
    send_disconnect_packet();

    running = false;
    if (listening.joinable()) listening.join();

    spdlog::info("Client shutting down.");
}

void Client::render_state(uint64_t render_time) {
    // trim state buffer
    std::lock_guard<std::mutex> state_lock_guard(state_mutex);
    // force second element to be after render_time and
    while (game_state_buffer.size() >= 2 &&
           game_state_buffer[1].header.timestamp <= render_time) {
        game_state_buffer.pop_front();
    }

    // draw the lerped states
    auto &a = game_state_buffer[0];
    auto &b = game_state_buffer[1];
    float t = (render_time - a.header.timestamp) /
              float(b.header.timestamp - a.header.timestamp);

    // draw other players in different color
    for (size_t i = 0; i < b.num_players; ++i) {
        const PlayerState &player_b = b.players[i];
        // try find this player in previous frame (A)
        auto itr = std::find_if(a.players.begin(),
                                a.players.end(),
                                [&player_b](const PlayerState &state) {
                                    return state.id == player_b.id;
                                });
        // default is latest frame (B)
        PlayerState resolved_player_state = player_b;
        // if existed on last frame, lerp
        if (itr != a.players.end()) {
            resolved_player_state = player_lerp(*itr, player_b, t);
        }
        // draw others in red
        Color color = {207, 87, 80, 255};
        // if this players data
        if (player_b.id == b.client_id) {
            client_id = b.client_id; // save it for any other cases
            // update camera for next frame
            camera.target.x +=
                (resolved_player_state.rect.x +
                 resolved_player_state.rect.width / 2.0f - camera.target.x) *
                0.05f;
            camera.target.y +=
                (resolved_player_state.rect.y +
                 resolved_player_state.rect.height / 2.0f - camera.target.y) *
                0.05f;
            // update color
            color = WHITE;
        }
        player_render(
            resolved_player_state, player_texture, health_bar_texture, color);
    }
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
                if (header->type == PacketType::GAME_STATE) {
                    GameStatePacket *gsp =
                        get_packet_data<GameStatePacket>(packet);
                    std::lock_guard<std::mutex> lock_guard(state_mutex);
                    game_state_buffer.push_back(*gsp);
                } else if (header->type == PacketType::BULLET) {
                    BulletStatePacket *bsp =
                        get_packet_data<BulletStatePacket>(packet);
                    bullet_state_buffer.push_back(*bsp);
                }

                // // bullet packet
                // else if (header->type == PacketType::BULLET) {
                //     std::vector<BulletPacket> enemy_bullets;
                //     get_bullet_data(packet, enemy_bullets);
                //     const std::lock_guard<std::mutex> bullet_lock(
                //         bullets_mutex);
                //
                //     auto &last_enemy_bullet =
                //         latest_enemy_bullet[header->sender];
                //     // update if timestamp is more recent
                //     if (header->timestamp > last_enemy_bullet.first) {
                //         last_enemy_bullet.first = header->timestamp;
                //         last_enemy_bullet.second = enemy_bullets;
                //     }
                // }
                //
                // // bullet collision packet
                // else if (header->type == PacketType::BULLET_COLLISION) {
                //     // WARN: we lose a UDP packet, player doesn't lose as
                //     much player->health -= 0.2f;
                // }
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

void Client::send_disconnect_packet() {
    ClientPacket p{.header = {.timestamp = timestamp,
                              .type = PacketType::CLIENT_DISCONNECT},
                   .x = 0.0f,
                   .y = 0.0f};
    send_client_packet(create_packet_from<ClientPacket>(p));
}

void Client::send_input_packet() {
    InputPacket input;
    input.left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    input.right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    input.up = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
    input.down = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
    input.mouse_down = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    // convert mouse position to world coordinates
    input.mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera);

    input.header.type = PacketType::INPUT;
    input.header.timestamp = get_now_millis();

    Packet packet = create_packet_from<InputPacket>(input);

    // send actual packet
    sendto(sock,
           packet.data(),
           packet.size(),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}
