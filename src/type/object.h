#ifndef GAME_OBJECT_TYPE_H
#define GAME_OBJECT_TYPE_H

#include "generic.h"

struct ObjectType : GenericType
{
  std::map<std::string, int> biomes;
  bool canExistIn(std::string biomeName) { return biomes[biomeName] == 1; }
};

#endif