#include "car.hpp"

#include <raylib.h>

#include <cmath>

Car::Car(const Rectangle &rect) : rect(rect) {}

Car::~Car() {}

void Car::input(float dt) {
    const float factor = 180.0f;
    // other way since y down
    if (IsKeyDown(KEY_LEFT)) {
        direction -= factor * dt;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        direction += factor * dt;
    }
    // if space is pressed we need to go forwards
    // TODO: acceleration
    forward = false;
    if (IsKeyDown(KEY_SPACE)) {
        forward = true;
        speed += ACCELERATION * dt;
        if (speed > SPEED) {
            speed = SPEED;
        }
    } else {
        speed -= ACCELERATION * dt;
        if (speed < 0.0f) {
            speed = 0.0f;
        }
    }
}

void Car::render() {
    DrawTexturePro(texture,
                   {0.0f, 0.0f, (float)texture.width, (float)texture.height},
                   rect,
                   {(float)texture.width / 2.0f, (float)texture.height / 2.0f},
                   direction,
                   WHITE);
}

void Car::update(float dt) {
    if (forward) {
        Vector2 vel;
        // since it's facing up by default
        float actual_dir = direction - 90.0f;
        vel.x = std::cos(actual_dir * PI / 180.0f) * speed;
        vel.y = std::sin(actual_dir * PI / 180.0f) * speed;
        rect.x += vel.x * dt;
        rect.y += vel.y * dt;
    }
}
