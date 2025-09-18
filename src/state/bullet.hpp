#ifndef STATE_BULLET_HPP
#define STATE_BULLET_HPP

#include <raylib.h>

#include "map/map.hpp"

constexpr static float BULLET_SIZE = 6.0f;

struct BulletState {
    int sender = 0;
    Vector2 pos, vel;
};

bool bullet_update(BulletState &b, float dt, GameMap &map);

void bullet_render(const Vector2 &pos, Texture bullet_texture);

#endif // STATE_BULLET_HPP
