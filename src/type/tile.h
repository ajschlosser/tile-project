#ifndef GAME_TILE_TYPE_H
#define GAME_TILE_TYPE_H

#include "generic.h"

struct TileType : GenericType
{
  TileType () {}
  TileType (Sprite* s, std::string n)
  {
    sprite = s;
    name = n;
  }
};

#endif