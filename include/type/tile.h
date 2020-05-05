#ifndef GAME_TILE_TYPE_H
#define GAME_TILE_TYPE_H

#include "generic.h"

struct TileType : GenericType
{
  TileType () {}
  TileType (std::string n) { this->name = n; }
};

#endif