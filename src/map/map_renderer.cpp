#include "map_renderer.hpp"

#include <raylib.h>

MapRenderer::MapRenderer(GameMap *map, const std::string &tileset_path)
    : map(map) {
    std::string path;

    for (size_t i = 0; i < map->tilesetCollection.size(); ++i) {
        const auto &tileset = map->tilesetCollection[i];
        path = tileset.image.source;
        tileset_textures.push_back(
            LoadTexture((tileset_path + "/" + path).c_str()));
    }
    setup();
}

MapRenderer::~MapRenderer() {
    for (auto &t : tileset_textures) {
        UnloadTexture(t);
    }
    for (auto &layer : layers) {
        UnloadRenderTexture(layer);
    }
}

void MapRenderer::setup() {
    unsigned int tile_width = map->tileWidth;
    unsigned int tile_height = map->tileHeight;
    unsigned int layer_width_pix = tile_width * map->width;
    unsigned int layer_height_pix = tile_height * map->height;

    for (unsigned int i = 0; i < map->layerCollection.size(); ++i) {
        layers.push_back(LoadRenderTexture(layer_width_pix, layer_height_pix));
    }

    for (unsigned int z = 0; z < map->layerCollection.size(); ++z) {
        tmxparser::Layer &layer = map->layerCollection[z];
        auto &framebuffer = layers[z];

        BeginTextureMode(framebuffer);

        for (size_t tile_idx = 0; tile_idx < layer.tiles.size(); ++tile_idx) {
            const tmxparser::Tile &tile = layer.tiles[tile_idx];
            const tmxparser::Tileset &tileset =
                map->tilesetCollection[tile.tilesetIndex];
            // find location of first pixel of tile
            unsigned int row, col, start_x, start_y;
            col = tile_idx % layer.width; // col in tile units
            row = tile_idx / layer.width; // row in tile units
            start_x = col * tile_width;   // x offset in pixels
            start_y = row * tile_height;  // y offset in pixels

            if (tile.gid == 0) {
                continue;
            }
            unsigned int gid = tile.tileFlatIndex;
            Rectangle src((gid % tileset.colCount) * tileset.tileWidth,
                          ((int)(gid / tileset.colCount)) * tileset.tileHeight,
                          tileset.tileWidth,
                          tileset.tileHeight);
            Rectangle dest(start_x, start_y, tile_width, tile_height);
            DrawTexturePro(
                tileset_textures[0], src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
        }
        // unbind fbo
        EndTextureMode();
    }
}

void MapRenderer::render() {
    for (unsigned int z = 0; z < map->layerCollection.size(); ++z) {
        auto &layer = map->layerCollection[z];
        if (layer.visible) {
            auto &tex = layers[z];
            // DrawTexture(tex.texture, 0.0f, 0.0f, WHITE);
            DrawTextureRec(tex.texture,
                           {0.0f,
                            0.0f,
                            (float)tex.texture.width,
                            (float)-tex.texture.height},
                           {0.0f, 0.0f},
                           WHITE);
        }
    }
}
