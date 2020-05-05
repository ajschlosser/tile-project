#ifndef GAME_TERRAIN_OBJECT_H
#define GAME_TERRAIN_OBJECT_H

#include "tile.h"

struct TerrainObject : Tile
{
  TerrainType* terrainType;
  TerrainObject () { type = tileObject::TERRAIN; }
  TerrainObject (int x, int y, int z, BiomeType* b, TerrainType* t)
  {
    type = tileObject::TERRAIN;
    this->x = x;
    this->y = y;
    this->z = z;
    biomeType = b;
    terrainType = t;
  }
};

#endif