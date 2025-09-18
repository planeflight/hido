#ifndef STATE_BULLET_HPP
#define STATE_BULLET_HPP

#include <raylib.h>

#include "map/map.hpp"

constexpr float BULLET_SIZE = 6.0f;

struct BulletState {
    uint64_t timestamp = 0;
    int sender = 0, id = -1;
    Vector2 pos, vel;
};

bool bullet_update(BulletState &b, float dt, GameMap &map);

void bullet_render(const Vector2 &pos, Texture bullet_texture, Color color);

#endif // STATE_BULLET_HPP
