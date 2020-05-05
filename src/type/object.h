#ifndef GAME_OBJECT_TYPE_H
#define GAME_OBJECT_TYPE_H

#include "generic.h"

struct ObjectType : GenericType
{
  std::map<std::string, int> biomes;
  ObjectType() {};
  ObjectType(
    std::string name,
    bool impassable,
    float multiplier,
    bool clusters,
    std::map<int, std::map<int, Sprite*>> animationMap,
    int animationSpeed,
    std::map<std::string, int> biomes
  )
  {
    this->name = name;
    this->impassable = impassable;
    this->multiplier = multiplier;
    this->clusters = clusters;
    this->animationMap = animationMap;
    this->animationSpeed = animationSpeed;
    this->biomes = biomes;
  }
  bool canExistIn(std::string biomeName) { return biomes[biomeName] == 1; }
};

#endif