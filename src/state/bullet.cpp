#include "bullet.hpp"

#include <raylib.h>

#include "map/map.hpp"
bool bullet_update(BulletState &b, float dt, GameMap &map) {
    b.pos.x += b.vel.x * dt;
    b.pos.y += b.vel.y * dt;

    std::vector<tmxparser::Tile *> collided_tiles;
    std::vector<unsigned int> collided_tile_indices;
    Rectangle rect{b.pos.x, b.pos.y, BULLET_SIZE, BULLET_SIZE};

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
                return true;
            }
        }
    }
    return false;
}

void bullet_render(const Vector2 &pos, Texture bullet_texture) {
    DrawTexturePro(
        bullet_texture,
        {0.0f, 0.0f, (float)bullet_texture.width, (float)bullet_texture.height},
        {pos.x, pos.y, BULLET_SIZE, BULLET_SIZE},
        {0.0f, 0.0f},
        0.0f,
        WHITE);
}
