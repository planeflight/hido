#ifndef HIDO_STATE_PLAYER_HPP
#define HIDO_STATE_PLAYER_HPP

#include <raylib.h>

#include "map/map.hpp"

constexpr float PLAYER_SPEED = 80.0f, PLAYER_WIDTH = 8.0f,
                PLAYER_HEIGHT = 12.0f;

struct PlayerState {
    Rectangle rect{0.0f, 0.0f, 0.0f, 0.0f};
    float health = 1.0f;
    int8_t id = -1;
};

void player_render(const PlayerState &p,
                   Texture player_texture,
                   Texture health_texture,
                   Color color);

void player_update(PlayerState &p, const Vector2 &vel, float dt, GameMap &map);

PlayerState player_lerp(const PlayerState &a, const PlayerState &b, float t);

#endif // HIDO_STATE_PLAYER_HPP
