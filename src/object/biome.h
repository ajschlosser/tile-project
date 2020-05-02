#ifndef GAME_BIOME_OBJECT_H
#define GAME_BIOME_OBJECT_H

#include "../type/biome.h"

struct BiomeObject
{
  int x;
  int y;
  BiomeType* biomeType;
  int type;
  BiomeObject () { type = tileObject::BIOME; }
};

#endif