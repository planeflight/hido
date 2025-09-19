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
#include <mutex>
#include <thread>

#include "map/map.hpp"
#include "map/map_renderer.hpp"
#include "network.hpp"
#include "state/bullet.hpp"
#include "state/player.hpp"

Client::Client(const std::string &addr, uint32_t port) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr.c_str(), &serv_addr.sin_addr);

    if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        spdlog::error("Error opening client connection.");
        running = false;
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
    if (!running) return;
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
        // network updates

        // get input and send packet
        send_input_packet();
        timestamp++;

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
            render_bullets(render_time);
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
    std::lock_guard<std::mutex> state_lock_guard(state_mutex);
    // trim state buffer
    // force second element to be after render_time
    game_state_buffer.remove_unused(render_time);

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

void Client::render_bullets(uint64_t render_time) {
    std::lock_guard<std::mutex> state_lock_guard(state_mutex);
    bullet_state_buffer.remove_unused(render_time);
    auto &a = bullet_state_buffer[0];
    auto &b = bullet_state_buffer[1];
    for (size_t i = 0; i < b.num_bullets; ++i) {
        auto &bullet_b = b.bullets[i];
        // find in a
        auto itr = std::find_if(b.bullets.begin(),
                                b.bullets.end(),
                                [&bullet_b](BulletPacket &bp) -> bool {
                                    return bullet_b.id == bp.id;
                                });
        Color color = WHITE;
        if (client_id >= 0 && b.bullets[i].sender != client_id) {
            color = Color{50, 20, 235, 255};
        }
        // if found
        if (itr != b.bullets.end()) {
            float t = (render_time - a.header.timestamp) /
                      float(b.header.timestamp - a.header.timestamp);
            bullet_render(
                Vector2Lerp(itr->pos, bullet_b.pos, t), bullet_texture, color);
        } else {
            bullet_render(bullet_b.pos, bullet_texture, color);
        }
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
                    std::lock_guard<std::mutex> lock_guard(state_mutex);
                    bullet_state_buffer.push_back(*bsp);
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
