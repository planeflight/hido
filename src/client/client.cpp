#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <thread>

#include "network.hpp"

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
    InitWindow(WIDTH, HEIGHT, "KARTO");

    player = std::make_unique<Car>(
        Rectangle{WIDTH / 2 - 28.0f, HEIGHT - 130.0f, 56.0f, 84.0f});
    Texture player_texture = LoadTexture("./res/car.png");
    player->texture = player_texture;
    Texture background_texture = LoadTexture("./res/map.png");

    Camera2D camera;
    camera.zoom = 1.0f;
    camera.offset = {WIDTH / 2.0f, HEIGHT * 0.75f};
    camera.target = {player->rect.x + player->rect.width / 2.0f,
                     player->rect.y + player->rect.height / 2.0f};
    camera.rotation = 0.0f;

    float last_t = GetTime();
    while (!WindowShouldClose()) {
        float dt = (GetTime() - last_t);
        last_t = GetTime();

        // input
        player->input(dt);

        // update
        player->update(dt);

        camera.target.x += (player->rect.x - camera.target.x) * 0.05f;
        camera.target.y += (player->rect.y - camera.target.y) * 0.05f;
        camera.rotation += (-player->direction - camera.rotation) * 0.05f;

        // network updates
        send_update_packet();

        // render
        BeginDrawing();
        ClearBackground(WHITE);
        BeginMode2D(camera);

        DrawTexturePro(background_texture,
                       {0.0f,
                        0.0f,
                        (float)background_texture.width,
                        (float)background_texture.height},
                       {0.0f,
                        0.0f,
                        background_texture.width * 2.0f,
                        background_texture.height * 2.0f},
                       {0.0f, 0.0f},
                       0.0f,
                       WHITE);
        player->render();

        // draw other clients
        mutex.lock();
        for (const auto &client_packet : clients) {
            Car c{Rectangle{client_packet.x, client_packet.y, 56.0f, 84.0f}};
            c.direction = client_packet.direction;
            c.texture = player_texture;
            c.render();
        }
        mutex.unlock();

        EndMode2D();
        EndDrawing();
    }
    // broadcast disconnect
    send_disconnect_packet();

    UnloadTexture(background_texture);
    UnloadTexture(player_texture);
    running = false;
    if (listening.joinable()) listening.join();

    spdlog::info("Client shutting down.");
}

void Client::listen_thread() {
    UpdatePacket packet;
    pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;

    while (running) {
        // non-blocking allows disconnect
        if (poll(fds, 1, 100) < 0) {
            spdlog::error("Polling error.");
            continue;
        }
        if (fds[0].revents & POLLIN) {
            sockaddr_in from{};
            socklen_t len = sizeof(from);
            int n = recvfrom(
                sock, &packet, sizeof(packet), 0, (sockaddr *)&from, &len);
            if (n <= 0) {
                spdlog::error("Failed to receive packet.");
                continue;
            }
            auto client_itr = std::find_if(clients.begin(),
                                           clients.end(),
                                           [&packet](const UpdatePacket &p) {
                                               return p.idx == packet.idx;
                                           });
            mutex.lock();
            // update packet if found and timestamp is updated
            if (client_itr != clients.end()) {
                if (client_itr->header.type == PacketType::DISCONNECT) {
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
                clients.push_back(packet);
            }
            mutex.unlock();
        }
    }
}

void Client::send_update_packet() {
    UpdatePacket packet{
        .header = {.timestamp = timestamp++, .type = PacketType::UPDATE},
        .x = player->rect.x,
        .y = player->rect.y,
        .direction = player->direction};
    sendto(sock,
           &packet,
           sizeof(packet),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}

void Client::send_disconnect_packet() {
    UpdatePacket packet{
        .header = {.timestamp = timestamp++, .type = PacketType::DISCONNECT}
        // don't bother with the rest
    };
    sendto(sock,
           &packet,
           sizeof(packet),
           0,
           (sockaddr *)&serv_addr,
           sizeof(serv_addr));
}
