#ifndef GAME_TILE_OBJECT_H
#define GAME_TILE_OBJECT_H

#include "../type/type.h"
#include "../timer.h"
#include "../uuid.h"

namespace tileObject
{
  enum directions
  {
    UP        = 0x01,
    DOWN      = 0x02,
    LEFT      = 0x04,
    RIGHT     = 0x08
  };
  enum types
  {
    BIOME       = 0x01,
    MOB         = 0x02,
    WORLD       = 0x04,
    SIMULATED   = 0x08,
    TERRAIN     = 0x16,
    TILE        = 0x32
  };
}

struct Tile
{
  int z;
  int x;
  int y;
  BiomeType* biomeType;
  TerrainType* terrainType;
  Sprite* sprite;
  Timer animationTimer;
  int animationFrame;
  int animationSpeed;
  bool isAnimated() { return animationSpeed > 0; }
  bool seen;
  bool initialized;
  int type;
  int direction;
  Tile() : seen(false), initialized(false), direction(tileObject::DOWN) { type = tileObject::TILE; }
  Tile (int x, int y, int z) { this->x = x; this->y = y; this->z = z; type = tileObject::TILE; }
  std::tuple<int, int, int, int> getPosition() { return std::make_tuple(z, x, y, direction); }
};

#endif