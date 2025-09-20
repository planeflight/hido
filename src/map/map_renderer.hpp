#ifndef HIDO_MAP_MAPRENDERER_HPP
#define HIDO_MAP_MAPRENDERER_HPP

#include <raylib.h>

#include <string>
#include <vector>

#include "map.hpp"

/**
 * Renders the tiled map by converting each layer into 1 texture for much faster
 * rendering
 */
class MapRenderer {
  public:
    MapRenderer(GameMap *map, const std::string &tileset_path);
    virtual ~MapRenderer();
    virtual void render();

  protected:
    virtual void setup();
    GameMap *map = nullptr;
    // framebuffers
    std::vector<RenderTexture2D> layers;

    std::vector<Texture> tileset_textures;
};

#endif // HIDO_MAP_MAPRENDERER_HPP
