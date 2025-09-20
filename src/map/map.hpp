#ifndef HIDO_MAP_MAP_HPP
#define HIDO_MAP_MAP_HPP

#include <libtmx-parser/tmxparser.h>
#include <raylib.h>

#include <functional>
#include <string>
#include <vector>

/**
 * Stores a tiled map
 */
class GameMap : public tmxparser::TmxMap {
  public:
    GameMap(const std::string &file_path, const std::string &tileset_path);
    virtual ~GameMap() = default;

    /**
     * Returns if there is a collision between the surrounding rects
     * @param rect intersection test rect
     * @param collided_tiles vector of pointers to tiles that intersect
     * with the given rect
     * @param collided_tile_indices vector of ints containing the location
     * of the collided tiles in the layer vector
     */
    void get_intersect_rects(const Rectangle &rect,
                             std::vector<tmxparser::Tile *> &collided_tiles,
                             std::vector<unsigned int> &collided_tile_indices);

    /**
     * Returns if there is a collision between the surrounding rects
     * @param rect intersection test rect
     * @param collided_tiles vector of pointers to tiles that intersect
     * with the given rect
     * @param collided_tile_indices vector of ints containing the location
     * of the collided tiles in the layer vector
     * @param func a function that returns whether or not the tile should be
     * counted (i.e. return false if it should be passed through)
     */

    void get_intersect_rects(
        const Rectangle &rect,
        std::vector<tmxparser::Tile *> &collided_tiles,
        std::vector<unsigned int> &collided_tile_indices,
        std::function<bool(tmxparser::Tile *, unsigned int)> func);

    /**
     * Sets the tile rectangle based off of the tileIdx
     * @param rect reference to the rect to be changed
     * @param tile_idx index of the tile in the 1d tile vector
     */
    virtual void set_tile_rect(Rectangle &rect, unsigned int tile_idx);

    /**
     * Checks if a tile has the given property
     * @param tile tile to check property
     * @param property string of property to check
     * @return out value in property map
     * @return if the tile has the given property
     */
    virtual bool contains_property(const tmxparser::Tile &tile,
                                   const std::string &property,
                                   std::string &out);

    /**
     * Finds all the tiles on the map with given property
     * @param property the tile property to check
     * @returns tiles_properties the vector of the tiles with that property
     * @returns tiles_properties_idx the vector of the indices of those tiles
     */
    virtual void get_tiles_with_property(
        const std::string &property,
        std::vector<tmxparser::Tile *> &tiles_properties,
        std::vector<int> &tiles_properties_idx);

    /**
     * @param x coord in tile units
     * @param y coord in tile units
     * @param layer coord in tile units
     * @returns the tile at the given position
     */
    virtual tmxparser::Tile &get_tile_at(int x, int y, int layer) {
        return layerCollection[layer].tiles[y * width + x];
    }
};

#endif // HIDO_MAP_MAP_HPP
