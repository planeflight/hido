/*
The MIT License (MIT)

Copyright (c) 2014 Stephen Damm - shinhalsafar@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _LIB_TMX_PARSER_H_
#define _LIB_TMX_PARSER_H_

#include <string>

#if defined __GXX_EXPERIMENTAL_CXX0X__ || (_MSC_VER >= 1800)
#include <unordered_map>
template <typename TKey, typename TValue>
struct Map {
    typedef std::unordered_map<TKey, TValue> type;
};
#else
#include <map>
template <typename TKey, typename TValue>
struct Map {
    typedef std::map<TKey, TValue> type;
};
#endif

#include "tinyxml2/tinyxml2.h"

#include <vector>

namespace tmxparser {

enum TmxReturn {
    kSuccess,
    kErrorParsing,
    kMissingRequiredAttribute,
    kMissingMapNode,
    kMissingDataNode,
    kMissingTilesetNode,
    kMalformedPropertyNode,
    kInvalidTileIndex,
    kUnknownTileIndices,
};

typedef Map<std::string, std::string>::type PropertyMap_t;

typedef unsigned int TileId_t;

enum Orientation {
    kOrthogonal = 0,
    kIsometric,
    kStaggered
};

enum ShapeType {
    kPolygon,
    kPolyline,
    kEllipse,
    kSquare,
};

struct Rect {
    float u;
    float v;
    float u2;
    float v2;
};

struct Property {
    std::string name;
    std::string value;
};

struct AnimationFrame {
    TileId_t tileId;
    float duration;
};

typedef std::vector<AnimationFrame> AnimationFrameCollection_t;

typedef std::pair<float, float> TmxShapePoint;

typedef std::vector<TmxShapePoint> TmxShapePointCollection_t;

struct Object {
    std::string name;
    std::string type;
    float x;
    float y;
    float width;
    float height;
    float rotation;
    unsigned int referenceGid;
    bool visible;
    PropertyMap_t propertyMap;
    ShapeType shapeType;
    TmxShapePointCollection_t shapePoints;
};

typedef std::vector<Object> ObjectCollection_t;

struct ObjectGroup {
    std::string name;
    std::string color;
    float opacity;
    bool visible;
    PropertyMap_t propertyMap;
    ObjectCollection_t objects;
};

typedef std::vector<ObjectGroup> ObjectGroupCollection_t;

struct TileDefinition {
    TileId_t id;
    PropertyMap_t propertyMap;
    AnimationFrameCollection_t animations;
    ObjectGroupCollection_t objectgroups;
};

typedef Map<unsigned int, TileDefinition>::type TileDefinitionMap_t;

struct Offset {
    int x;
    int y;
};

struct Image {
    std::string format;
    std::string source;
    std::string transparentColor;
    unsigned int width;
    unsigned int height;
};

struct ImageLayer {
    std::string name;
    unsigned int x;
    unsigned int y;
    unsigned int widthInTiles;
    unsigned int heightInTiles;
    float opacity;
    bool visible;
    PropertyMap_t propertyMap;
    Image image;
};

typedef std::vector<ImageLayer> ImageLayerCollection_t;

struct Tileset {
    std::string source;
    unsigned int firstgid;
    std::string name;
    unsigned int tileWidth;
    unsigned int tileHeight;
    unsigned int tileSpacingInImage;
    unsigned int tileMarginInImage;
    Offset offset;

    unsigned int rowCount; /// based on image width and tile spacing/margins
    unsigned int colCount; /// based on image height and tile spacing/margins

    Image image;
    TileDefinitionMap_t tileDefinitions;
};

typedef std::vector<Tileset> TilesetCollection_t;

struct Tile {
    unsigned int gid; // corresponding tile in tileset 0=blank, >= 1=tile
    unsigned int tilesetIndex;
    unsigned int tileFlatIndex; // pos of tile in tileset
    bool flipX, flipY, flipDiagonal;

    bool operator==(const Tile &other) const {
        return gid == other.gid && tilesetIndex == other.tilesetIndex && tileFlatIndex == other.tileFlatIndex && flipX == other.flipX && flipY == other.flipY && flipDiagonal == other.flipDiagonal;
    }
};

typedef std::vector<Tile> TileCollection_t;

struct Layer {
    std::string name;
    unsigned int width;
    unsigned int height;
    float opacity;
    bool visible;
    PropertyMap_t propertyMap;
    TileCollection_t tiles;
};

typedef std::vector<Layer> LayerCollection_t;

struct TmxMap {
    std::string version;
    Orientation orientation;
    unsigned int width;
    unsigned int height;
    unsigned int tileWidth;
    unsigned int tileHeight;
    std::string backgroundColor;
    std::string renderOrder;
    PropertyMap_t propertyMap;
    TilesetCollection_t tilesetCollection;
    LayerCollection_t layerCollection;
    ObjectGroupCollection_t objectGroupCollection;
    ImageLayerCollection_t imageLayerCollection;
};

/**
 * Parse a tmx from a filename.
 * @param fileName Relative or Absolute filename to the TMX file to load.
 * @param outMap An allocated TmxMap object ready to be populated.
 * @return kSuccess on success.
 */
TmxReturn parseFromFile(const std::string &fileName, TmxMap *outMap, const std::string &tilesetPath);

/**
 * Parse a tmx file from memory.  Useful for Android or IOS.
 * @param data Tmx file in memory, still in its xml format just already loaded.
 * @param length Size of the data buffer.
 * @param outMap An allocated TmxMap object ready to be populated.
 * @return kSuccess on success.
 */
TmxReturn parseFromMemory(void *data, size_t length, TmxMap *outMap, const std::string &tilesetPath);

/**
 * Takes a tileset and an index with that tileset and generates OpenGL/DX ready texture coordinates.
 * @param tileset A tileset to use for generating coordinates.
 * @param tileFlatIndex The flat index into the tileset.
 * @param pixelCorrection The amount to use for pixel correct, 0.5f is typical, see 'half-pixel correction'.
 * @param outRect Contains the four corners of the tile.
 * @return kSuccess on success.
 */
TmxReturn calculateTileCoordinatesUV(const Tileset &tileset, unsigned int tileFlatIndex, float pixelCorrection, bool flipY, Rect &outRect);

} // namespace tmxparser

#endif /* _LIB_TMX_PARSER_H_ */
