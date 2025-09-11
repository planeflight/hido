#include "bullet.hpp"

#include <raylib.h>

#include "map/map.hpp"

Bullet::Bullet(const Rectangle &rect, const Vector2 &vel) : rect(rect) {
    this->vel = vel;
    this->vel.x *= SPEED;
    this->vel.y *= SPEED;
}

void Bullet::render(Texture texture) {
    DrawTexturePro(texture,
                   {0.0f, 0.0f, (float)texture.width, (float)texture.height},
                   rect,
                   {0.0f, 0.0f},
                   0.0f,
                   WHITE);
}

void Bullet::update(float dt, GameMap &map, const std::vector<Player> &others) {
    rect.x += vel.x * dt;
    rect.y += vel.y * dt;

    std::vector<tmxparser::Tile *> collided_tiles;
    std::vector<unsigned int> collided_tile_indices;

    map.get_intersect_rects(
        rect,
        collided_tiles,
        collided_tile_indices,
        [&](tmxparser::Tile *tile, unsigned int tile_idx) -> bool {
            (void)tile_idx;
            std::string out;
            return map.contains_property(*tile, "blocked", out);
        });
    Rectangle test_rect;

    for (size_t i = 0; i < collided_tiles.size(); ++i) {
        map.set_tile_rect(test_rect, collided_tile_indices[i]);
        if (CheckCollisionRecs(test_rect, rect)) {
            std::string out;
            bool blocked =
                map.contains_property(*collided_tiles[i], "blocked", out);
            if (blocked) {
                destroy = true;
                return;
            }
        }
    }
}
