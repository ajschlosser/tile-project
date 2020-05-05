#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "json/json.h"
#include "objects.h"

#include <fstream>
#include <tuple>
#include <vector>


namespace config
{
typedef std::map<int, std::map<int, Sprite*>> animationMap;

struct ConfigurationController
{
  int gameSize;
  int tileSize;
  int spriteSize;
  int chunkFuzz;
  objects::mobTypesMap mobTypes;
  objects::objectTypesMap objectTypes;
  objects::biomeTypesMap biomeTypes;
  std::map<int, std::map<std::string, BiomeType*>> biomeLevelMap;
  std::map<int, std::vector<std::string>> biomeTypeProbabilities;
  std::vector<std::string> biomeTypeKeys;
  std::vector<std::string> terrainTypesKeys;
  objects::terrainTypesMap terrainTypes;
  objects::tileTypesMap tileTypes;
  Json::Value configJson;
  std::map<std::string, Sprite> sprites;
  ConfigurationController () {}
  ConfigurationController (std::string, std::map<std::string, Sprite>);
  animationMap configureAnimationMap (int, std::string);
  std::tuple<
    objects::biomeTypesMap*,
    std::vector<std::string>*,
    objects::terrainTypesMap*,
    objects::mobTypesMap*,
    objects::objectTypesMap*,
    objects::tileTypesMap*
  > getTypeMaps()
  {
    return std::make_tuple(
      &biomeTypes, &biomeTypeKeys, &terrainTypes, &mobTypes, &objectTypes, &tileTypes
    );
  }
  BiomeType* getRandomBiomeType(int z = 0) { return &biomeTypes[biomeTypeProbabilities[z][std::rand() % biomeTypeProbabilities[z].size()]]; }
  TerrainType* getRandomTerrainType() { return &terrainTypes[terrainTypesKeys[std::rand() % terrainTypesKeys.size()]]; }
  TerrainType* getRandomTerrainType(std::string biomeType)
  {
    auto t = biomeTypes[biomeType].terrainTypes.at(std::rand() % biomeTypes[biomeType].terrainTypes.size());
    return &terrainTypes[t.first];
  }
  bool biomeExistsOnLevel(std::string name, int z) { return biomeLevelMap[z].find(name) != biomeLevelMap[z].end(); }
};

}

#endif