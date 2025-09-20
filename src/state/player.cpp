#include "player.hpp"

#include <raylib.h>

PlayerState::PlayerState() {
    rect = Rectangle{20.0f, 20.0f, PLAYER_WIDTH, PLAYER_HEIGHT};
}

void player_render(const PlayerState &p,
                   Texture player_texture,
                   Texture health_texture,
                   Color color) {
    DrawTexture(player_texture, p.rect.x, p.rect.y, color);
    // draw health bar
    const float max_width = 14.0f, max_height = 5.0f;
    const float bar_width = p.health * (max_width - 2.0f);

    Rectangle health_rect;
    health_rect.x = p.rect.x + p.rect.width / 2.0f - max_width / 2.0f;
    health_rect.y = p.rect.y - max_height - 2.0f;
    health_rect.width = max_width;
    health_rect.height = max_height;

    DrawRectangle(health_rect.x + 1.0f,
                  health_rect.y + 1.0f,
                  health_rect.width - 2.0f,
                  health_rect.height - 2.0f,
                  Color{185, 89, 61, 255});
    DrawRectangle(health_rect.x + 1.0f,
                  health_rect.y + 1.0f,
                  bar_width,
                  health_rect.height - 2.0f,
                  Color{150, 80, 250, 255});
    // finally draw the texture
    DrawTexture(health_texture, health_rect.x, health_rect.y, WHITE);
}

void player_update(PlayerState &p, const Vector2 &vel, float dt, GameMap &map) {
    p.rect.x += vel.x * dt;

    std::vector<tmxparser::Tile *> collided_tiles;
    std::vector<unsigned int> collided_tile_indices;

    map.get_intersect_rects(
        p.rect,
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
        if (CheckCollisionRecs(test_rect, p.rect)) {
            std::string out;
            bool blocked =
                map.contains_property(*collided_tiles[i], "blocked", out);
            if (blocked) {
                // left
                if (vel.x < 0.0f) {
                    p.rect.x = test_rect.x + test_rect.width;
                }
                // right
                else if (vel.x > 0.0f) {
                    p.rect.x = test_rect.x - p.rect.width;
                }
            }
        }
    }

    // resolve movement axes separately
    p.rect.y += vel.y * dt;
    collided_tiles.clear();
    collided_tile_indices.clear();
    map.get_intersect_rects(
        p.rect,
        collided_tiles,
        collided_tile_indices,
        [&](tmxparser::Tile *tile, unsigned int tile_idx) -> bool {
            (void)tile_idx;
            std::string out;
            return map.contains_property(*tile, "blocked", out);
        });
    for (size_t i = 0; i < collided_tiles.size(); ++i) {
        map.set_tile_rect(test_rect, collided_tile_indices[i]);
        if (CheckCollisionRecs(test_rect, p.rect)) {
            std::string out;
            bool blocked =
                map.contains_property(*collided_tiles[i], "blocked", out);
            if (blocked) {
                // up
                if (vel.y < 0.0f) {
                    p.rect.y = test_rect.y + test_rect.height;
                }
                // right
                else if (vel.y > 0.0f) {
                    p.rect.y = test_rect.y - p.rect.height;
                }
            }
        }
    }
}

PlayerState player_lerp(const PlayerState &a, const PlayerState &b, float t) {
    PlayerState state = a;
    state.rect.x = a.rect.x + t * (b.rect.x - a.rect.x);
    state.rect.y = a.rect.y + t * (b.rect.y - a.rect.y);
    state.health = a.health + t * (b.health - a.health);
    return state;
}
