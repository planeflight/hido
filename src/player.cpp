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
