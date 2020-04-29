#include "config.h"

ConfigurationController::ConfigurationController (std::string configFilePath, std::map<std::string, Sprite> s)
{

  sprites = s;
  std::ifstream configFile(configFilePath.c_str());
  configFile >> configJson;

  chunkFuzz = configJson["map"]["chunks"]["fuzz"].asInt();

  gameSize = configJson["gameSize"].asInt();
  tileSize = configJson["tileSize"].asInt();
  spriteSize = configJson["spriteSize"].asInt();

  for (auto i = 0; i < configJson["terrains"].size(); ++i)
  {
    std::string spriteName = configJson["terrains"][i]["sprite"].asString();
    std::string tileTypeName = configJson["terrains"][i]["name"].asString();
    bool impassable = configJson["terrains"][i]["impassable"].asBool();
    float multiplier = configJson["terrains"][i]["multiplier"].asFloat();
    std::vector<std::string> relatedObjectTypes;
    const Json::Value& relatedObjectsArray = configJson["terrains"][i]["objects"];
    for (int i = 0; i < relatedObjectsArray.size(); i++)
    {
      relatedObjectTypes.push_back(relatedObjectsArray[i].asString());
    }
    TileType tileType { &sprites[spriteName], tileTypeName };
    TerrainType terrainType { &sprites[spriteName], tileTypeName, relatedObjectTypes };
    terrainType.impassable = impassable;
    terrainType.multiplier = multiplier;
    tileTypes[tileType.name] = tileType;
    terrainTypes[terrainType.name] = terrainType;
    terrainTypesKeys.push_back(terrainType.name);
    SDL_Log("- Loaded '%s' terrain", tileTypeName.c_str());
  }
  for (auto i = 0; i < configJson["biomes"].size(); ++i)
  {
    BiomeType b;
    b.name = configJson["biomes"][i]["name"].asString();
    b.maxDepth = configJson["biomes"][i]["maxDepth"].asInt();
    b.minDepth = configJson["biomes"][i]["minDepth"].asInt();
    b.multiplier = configJson["biomes"][i]["multiplier"].asFloat();
    if (b.multiplier <= 0)
      b.multiplier = 1;
    const Json::Value& terrainsArray = configJson["biomes"][i]["terrains"];
    for (int i = 0; i < terrainsArray.size(); i++)
    {
      auto t = terrainsArray[i];
      auto m = t["multiplier"].asFloat();
      if (m <= 0)
        m = 1;
      b.terrainTypes[b.terrainTypes.size()] = { t["name"].asString(), m };
      for (int i = 0; i < 10 * m; i++)
        b.terrainTypeProbabilities.push_back(t["name"].asString());
    }
    biomeTypes[b.name] = b;
    biomeTypeKeys.push_back(b.name);
    for (int i = 0; i < 10 * b.multiplier; i++)
      biomeTypeProbabilities.push_back(b.name);
    SDL_Log("- Loaded '%s' biome", b.name.c_str());
  }
  for (auto i = 0; i < configJson["objects"].size(); ++i)
  {
    std::string spriteName = configJson["objects"][i]["sprite"].asString();
    std::string objectTypeName = configJson["objects"][i]["name"].asString();
    bool impassable = configJson["objects"][i]["impassable"].asBool();
    const Json::Value& biomesArray = configJson["objects"][i]["biomes"];
    std::map<std::string, int> bM;
    for (int i = 0; i < biomesArray.size(); i++)
    {
      bM[biomesArray[i].asString()] = 1;
    }
    TileType tileType { &sprites[spriteName], objectTypeName };
    tileTypes[tileType.name] = tileType;
    ObjectType o { &sprites[spriteName], objectTypeName, impassable, configJson["objects"][i]["multiplier"].asFloat(), bM };
    objectTypes[objectTypeName] = o;
    SDL_Log("- Loaded '%s' object", objectTypeName.c_str());
  }
  for (auto i = 0; i < configJson["mobs"].size(); ++i)
  {
    std::string spriteName = configJson["mobs"][i]["sprite"].asString();
    std::string mobTypeName = configJson["mobs"][i]["name"].asString();
    const Json::Value& biomesArray = configJson["mobs"][i]["biomes"];
    std::map<std::string, int> bM;
    for (int i = 0; i < biomesArray.size(); i++)
    {
      bM[biomesArray[i].asString()] = 1;
    }
    SDL_Log("- Loaded '%s' mob", mobTypeName.c_str());
    TileType tileType { &sprites[spriteName], mobTypeName };
    tileTypes[tileType.name] = tileType;
    MobType mobType { &sprites[spriteName], mobTypeName, false, configJson["mobs"][i]["multiplier"].asFloat(), bM };
    mobTypes[mobType.name] = mobType;
  }
}