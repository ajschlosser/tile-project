#ifndef GAME_TERRAIN_TYPE_H
#define GAME_TERRAIN_TYPE_H

#include "generic.h"
#include <vector>

struct TerrainType : GenericType
{
  std::vector<std::string> objects;
  int objectFrequencyMultiplier;
  std::vector<std::string> objectTypeProbabilities;
  TerrainType () {}
  TerrainType (Sprite* sprite, std::string name, std::vector<std::string> objects)
  {
    this->sprite = sprite;
    this->name = name;
    this->objects = objects;
    this->objectFrequencyMultiplier = 1.0;
  }
  int getObjectFrequencyMultiplier() { if (objectFrequencyMultiplier > 0) return objectFrequencyMultiplier; else return 1; }
  std::string getRandomObjectTypeName()
  {
    return objectTypeProbabilities.at(rand() % objectTypeProbabilities.size());
  }
};

#endif