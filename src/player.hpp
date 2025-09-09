#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <raylib.h>

#include "map/map.hpp"

struct Player {
    explicit Player(const Rectangle &rect);
    ~Player();

    void input(float dt);
    void render();
    void update(float dt, GameMap &map);

    Rectangle rect;
    Texture texture;

    Vector2 vel;
    constexpr static float SPEED = 80.0f;
    constexpr static float ACCELERATION = 900.0f;
};

#endif // PLAYER_HPP
