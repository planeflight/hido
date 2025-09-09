#include "map.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <iostream>

GameMap::GameMap(const std::string &file_path,
                 const std::string &tileset_path) {
    tmxparser::TmxReturn ret = parseFromFile(file_path, this, tileset_path);
    if (ret != tmxparser::TmxReturn::kSuccess) {
        spdlog::error("Failed to load file: '{}'.");
    }
}

void GameMap::get_intersect_rects(
    const Rectangle &rect,
    std::vector<tmxparser::Tile *> &collided_tiles,
    std::vector<unsigned int> &collided_tile_indices) {
    get_intersect_rects(rect,
                        collided_tiles,
                        collided_tile_indices,
                        [&](tmxparser::Tile *, unsigned int) { return false; });
}

void GameMap::get_intersect_rects(
    const Rectangle &rect,
    std::vector<tmxparser::Tile *> &collided_tiles,
    std::vector<unsigned int> &collided_tile_indices,
    std::function<bool(tmxparser::Tile *, unsigned int)> func) {
    // reset the output vectors
    collided_tiles.clear();
    collided_tile_indices.clear();
    // make sure that the rectangle is not out of bounds
    if (rect.x < 0.0f || rect.x + rect.width > (float)width * tileWidth ||
        rect.y < 0.0f || rect.y + rect.height > (float)height * tileHeight) {
        return;
    }
    // find bounds to search
    unsigned int left, right, top, bottom;
    left = std::max(std::floor(rect.x / tileWidth), 0.0f);
    right =
        std::min(std::floor((rect.x + rect.width) / tileWidth), width - 1.0f);
    bottom = std::max(std::floor(rect.y / tileHeight), 0.0f);
    top = std::min(std::floor((rect.y + rect.height) / tileHeight),
                   height - 1.0f);
    for (auto &layer : layerCollection) {
        if (!layer.visible) continue;

        for (unsigned int y = bottom; y <= top; y++) {
            for (unsigned int x = left; x <= right; x++) {
                // no flip since map orientation is correct in the tmx file
                unsigned int tile_pos = y * layer.width + x;
                tmxparser::Tile &tile = layer.tiles[tile_pos];
                if (tile.gid != 0 && func(&tile, tile_pos)) {
                    collided_tiles.push_back(&tile);
                    collided_tile_indices.push_back(tile_pos);
                }
            }
        }
    }
}

void GameMap::set_tile_rect(Rectangle &rect, unsigned int tile_idx) {
    // convert tile_idx to correct width and height
    unsigned int actual_x = tile_idx % width;
    unsigned int actual_y = tile_idx / width;
    // now turn into rect
    rect.x = actual_x * tileWidth;
    rect.y = actual_y * tileHeight;
    rect.width = tileWidth;
    rect.height = tileHeight;
}

bool GameMap::contains_property(const tmxparser::Tile &tile,
                                const std::string &property,
                                std::string &out) {
    const tmxparser::Tileset &tileset = tilesetCollection[tile.tilesetIndex];
    const tmxparser::TileDefinitionMap_t &tile_def_map =
        tileset.tileDefinitions;
    // check if there is a definition for the tile in the tileset
    if (tile_def_map.count(tile.tileFlatIndex) == 0) {
        return false;
    }
    // check if one of the properties is the given property
    const tmxparser::TileDefinition &def = tile_def_map.at(tile.tileFlatIndex);
    if (def.propertyMap.count(property) == 0) return false;
    out = def.propertyMap.at(property);
    return true;
}

void GameMap::get_tiles_with_property(
    const std::string &property,
    std::vector<tmxparser::Tile *> &tiles_properties,
    std::vector<int> &tiles_properties_idx) {
    tiles_properties.clear();
    tiles_properties_idx.clear();
    for (auto &layer : layerCollection) {
        // if (!layer.visible) continue;
        auto &tiles = layer.tiles;
        // check all tiles in current layer
        // for the property
        for (size_t tile_gid = 0; tile_gid < tiles.size(); ++tile_gid) {
            auto &tile = tiles[tile_gid];
            if (tile.gid == 0) continue;
            // check if the tile has the property
            std::string out;
            if (contains_property(tile, property, out)) {
                tiles_properties.push_back(&tile);
                tiles_properties_idx.push_back(tile_gid);
            }
        }
    }
}
