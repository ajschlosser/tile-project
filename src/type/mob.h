#ifndef GAME_MOB_TYPE_H
#define GAME_MOB_TYPE_H

#include "object.h"

struct MobType : ObjectType
{
  MobType() {};
  MobType(
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
};

#endif