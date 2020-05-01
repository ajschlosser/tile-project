#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include "json/json.h"
#include "objects.h"

#include <fstream>
#include <tuple>
#include <vector>

struct ConfigurationController
{
  int gameSize;
  int tileSize;
  int spriteSize;
  int chunkFuzz;
  objects::mobTypesMap mobTypes;
  objects::objectTypesMap objectTypes;
  objects::biomeTypesMap biomeTypes;
  std::vector<std::string> biomeTypeProbabilities;
  std::map<int, std::map<std::string, BiomeType*>> biomeLevelMap;
  std::map<int, std::vector<std::string>> biomeTypeProbabilitiesLevels;
  std::vector<std::string> biomeTypeKeys;
  std::vector<std::string> terrainTypesKeys;
  objects::terrainTypesMap terrainTypes;
  objects::tileTypesMap tileTypes;
  Json::Value configJson;
  std::map<std::string, Sprite> sprites;
  ConfigurationController () {}
  ConfigurationController (std::string, std::map<std::string, Sprite>);
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
  BiomeType* getRandomBiomeType(int z = 0) { return &biomeTypes[biomeTypeProbabilitiesLevels[z][std::rand() % biomeTypeProbabilitiesLevels[z].size()]]; }
  //BiomeType* getRandomBiomeType() { return &biomeTypes[biomeTypeProbabilities[std::rand() % biomeTypeProbabilities.size()]]; }
  TerrainType* getRandomTerrainType() { return &terrainTypes[terrainTypesKeys[std::rand() % terrainTypesKeys.size()]]; }
  TerrainType* getRandomTerrainType(std::string biomeType)
  {
    auto t = biomeTypes[biomeType].terrainTypes.at(std::rand() % biomeTypes[biomeType].terrainTypes.size());
    return &terrainTypes[t.first];
  }
  bool biomeExistsOnLevel(std::string name, int z) { return biomeLevelMap[z].find(name) != biomeLevelMap[z].end(); }
};

#endif