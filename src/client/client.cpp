#include "client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <raylib.h>
#include <spdlog/spdlog.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <thread>

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

    std::string msg;
    SetTargetFPS(60);
    InitWindow(WIDTH, HEIGHT, "KARTO");

    player = std::make_unique<Car>(
        Rectangle{WIDTH / 2 - 28.0f, HEIGHT - 130.0f, 56.0f, 84.0f});
    player->texture = LoadTexture("./res/car.png");
    Texture background_texture = LoadTexture("./res/map.png");

    Camera2D camera;
    camera.zoom = 1.0f;
    camera.target = {player->rect.x + player->rect.width / 2.0f,
                     player->rect.y + player->rect.height / 2.0f};
    camera.rotation = 0.0f;

    float last_t = GetTime();
    while (!WindowShouldClose()) {
        // std::getline(std::cin, msg);
        // sendto(sock,
        //        msg.c_str(),
        //        msg.size(),
        //        0,
        //        (sockaddr *)&serv_addr,
        //        sizeof(serv_addr));
        float dt = (GetTime() - last_t);
        last_t = GetTime();

        // input
        player->input(dt);

        // update
        player->update(dt);

        camera.target.x += (player->rect.x - camera.target.x) * 0.05f;
        camera.target.y += (player->rect.y - camera.target.y) * 0.05f;
        camera.rotation += (-player->direction - camera.rotation) * 0.05f;
        ;

        // render
        BeginDrawing();
        ClearBackground(WHITE);
        BeginMode2D(camera);

        DrawTexture(background_texture, 0.0f, 0.0f, WHITE);
        player->render();

        EndMode2D();
        EndDrawing();
    }
    running = false;
    if (listening.joinable()) listening.join();

    spdlog::info("Client shutting down.");
}

void Client::listen_thread() {
    char buffer[1024];
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
                sock, buffer, sizeof(buffer), 0, (sockaddr *)&from, &len);
            if (n > 0) {
                buffer[n] = '\0';
                fmt::println("Server: {}", buffer);
            }
        }
    }
}
