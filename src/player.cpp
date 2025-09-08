#include "player.hpp"

#include <raylib.h>

#include <cmath>

Player::Player(const Rectangle &rect) : rect(rect) {}

Player::~Player() {}

void Player::input(float dt) {
    vel.x = 0.0f;
    vel.y = 0.0f;
    if (IsKeyDown(KEY_LEFT)) {
        vel.x = -SPEED;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        vel.x = SPEED;
    }

    if (IsKeyDown(KEY_DOWN)) {
        vel.y = SPEED;
    }
    if (IsKeyDown(KEY_UP)) {
        vel.y = -SPEED;
    }
}

void Player::render() {
    DrawTexture(texture, rect.x, rect.y, WHITE);
}

void Player::update(float dt) {
    rect.x += vel.x * dt;
    rect.y += vel.y * dt;
}
