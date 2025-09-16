#include "player.hpp"

#include <raylib.h>

#include <cmath>

#include "map/map.hpp"

Player::Player(const Rectangle &rect) : rect(rect) {}

Player::~Player() {}

void Player::input(float dt) {
    vel.x = 0.0f;
    vel.y = 0.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        vel.x = -SPEED;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        vel.x = SPEED;
    }

    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        vel.y = SPEED;
    }
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        vel.y = -SPEED;
    }
}

void Player::render() {
    DrawTexture(texture, rect.x, rect.y, WHITE);
    // draw health bar
    const float max_width = 14.0f, max_height = 5.0f;
    const float bar_width = health * (max_width - 2.0f);

    Rectangle health_rect;
    health_rect.x = rect.x + rect.width / 2.0f - max_width / 2.0f;
    health_rect.y = rect.y - max_height - 2.0f;
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
    DrawTexture(health_bar_texture, health_rect.x, health_rect.y, WHITE);
}

void Player::update(float dt, GameMap &map) {
    rect.x += vel.x * dt;

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
                // left
                if (vel.x < 0.0f) {
                    rect.x = test_rect.x + test_rect.width;
                    vel.x = 0.0f;
                }
                // right
                else if (vel.x > 0.0f) {
                    rect.x = test_rect.x - rect.width;
                    vel.x = 0.0f;
                }
            }
        }
    }

    // resolve movement axes separately
    rect.y += vel.y * dt;
    collided_tiles.clear();
    collided_tile_indices.clear();
    map.get_intersect_rects(
        rect,
        collided_tiles,
        collided_tile_indices,
        [&](tmxparser::Tile *tile, unsigned int tile_idx) -> bool {
            (void)tile_idx;
            std::string out;
            return map.contains_property(*tile, "blocked", out);
        });
    for (size_t i = 0; i < collided_tiles.size(); ++i) {
        map.set_tile_rect(test_rect, collided_tile_indices[i]);
        if (CheckCollisionRecs(test_rect, rect)) {
            std::string out;
            bool blocked =
                map.contains_property(*collided_tiles[i], "blocked", out);
            if (blocked) {
                // up
                if (vel.y < 0.0f) {
                    rect.y = test_rect.y + test_rect.height;
                    vel.y = 0.0f;
                }
                // right
                else if (vel.y > 0.0f) {
                    rect.y = test_rect.y - rect.height;
                    vel.y = 0.0f;
                }
            }
        }
    }
}
