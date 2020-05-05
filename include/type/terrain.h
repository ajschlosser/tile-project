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
  TerrainType(
    std::string name,
    std::vector<std::string> relatedObjectTypes,
    float objectFrequencyMultiplier,
    std::vector<std::string> relatedObjectTypeProbabilities,
    bool impassable,
    float multiplier,
    bool clusters
  )
  {
    this->name = name;
    this->objects = relatedObjectTypes;
    this->objectFrequencyMultiplier = objectFrequencyMultiplier;
    this->objectTypeProbabilities = relatedObjectTypeProbabilities;
    this->impassable = impassable;
    this->multiplier = multiplier;
    this->clusters = clusters;
  };
  int getObjectFrequencyMultiplier() { if (objectFrequencyMultiplier > 0) return objectFrequencyMultiplier; else return 1; }
  std::string getRandomObjectTypeName()
  {
    return objectTypeProbabilities.at(rand() % objectTypeProbabilities.size());
  }
};

#endif