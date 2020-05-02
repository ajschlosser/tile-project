#ifndef GAME_TERRAIN_OBJECT_H
#define GAME_TERRAIN_OBJECT_H

#include "tile.h"

struct TerrainObject : Tile
{
  TerrainType* terrainType;
  TerrainObject () { type = tileObject::TERRAIN; }
  TerrainObject (int x, int y, BiomeType* b, TerrainType* t)
  {
    type = tileObject::TERRAIN;
    this->x = x;
    this->y = y;
    biomeType = b;
    terrainType = t;
    sprite = t->sprite;
  }
};

#endif