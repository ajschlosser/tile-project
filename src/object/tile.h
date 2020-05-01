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
}

struct Tile
{
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
  Tile() : seen(false), initialized(false) {}
  Tile (int x, int y) { this->x = x; this->y = y; }
};

#endif