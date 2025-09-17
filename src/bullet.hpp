#ifndef BULLET_HPP
#define BULLET_HPP

#include <raylib.h>

#include <vector>

#include "map/map.hpp"

class Bullet {
  public:
    Bullet(const Rectangle &rect, const Vector2 &vel);

    void render(Texture texture);
    // void update(float dt, GameMap &map, const std::vector<Player> &others);

    bool get_destroy() const {
        return destroy;
    }

    Vector2 get_pos() const {
        return Vector2{rect.x, rect.y};
    }

  private:
    Rectangle rect;
    constexpr static float SPEED = 350.0f;

    Vector2 vel;
    bool destroy = false;
};

#endif // BULLET_HPP
