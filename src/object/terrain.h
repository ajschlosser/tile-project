#ifndef GAME_TERRAIN_OBJECT_H
#define GAME_TERRAIN_OBJECT_H

#include "tile.h"

struct TerrainObject : Tile
{
  TerrainType* terrainType;
  TerrainObject () {}
  TerrainObject (int x, int y, BiomeType* b, TerrainType* t)
  {
    this->x = x;
    this->y = y;
    biomeType = b;
    terrainType = t;
    sprite = t->sprite;
  }
};

#endif