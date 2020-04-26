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
  objects::mobTypesMap mobTypes;
  objects::objectTypesMap objectTypes;
  objects::biomeTypesMap biomeTypes;
  std::vector<std::string> biomeTypeKeys;
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
  BiomeType* getRandomBiomeType() { return &biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]]; }
};

#endif