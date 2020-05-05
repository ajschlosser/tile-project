#ifndef GAME_BIOME_TYPE_H
#define GAME_BIOME_TYPE_H

#include <map>
#include <string>
#include <vector>

struct BiomeType
{
  std::string name;
  int maxDepth;
  int minDepth;
  std::map<int, std::pair<std::string, float>> terrainTypes;
  float multiplier;
  std::vector<std::string> terrainTypeProbabilities;
  BiomeType () {}
  std::string getRandomTerrainTypeName() { return terrainTypeProbabilities.at(std::rand() % terrainTypeProbabilities.size()); }
};

#endif