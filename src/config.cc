#include "config.h"

using namespace config;

animationMap ConfigurationController::configureAnimationMap (int i, std::string n)
{
  animationMap m;
  if (configJson[n][i]["sprite"].isString())
  {
    m[tileObject::DOWN][0] = &sprites[configJson[n][i]["sprite"].asString()];
  }
  else if (configJson[n][i]["sprite"].isArray())
  {
    const Json::Value& animationArr = configJson[n][i]["sprite"];
    for (int i = 0; i < animationArr.size(); i++)
    {
      m[tileObject::DOWN][i] = &sprites[animationArr[i].asString()];
    }
  }
  else if (configJson[n][i]["sprite"].isObject())
  {
    if (configJson[n][i]["sprite"]["directions"].isObject())
    {
      const Json::Value& uArr = configJson[n][i]["sprite"]["directions"]["up"];
      const Json::Value& dArr = configJson[n][i]["sprite"]["directions"]["down"];
      const Json::Value& lArr = configJson[n][i]["sprite"]["directions"]["left"];
      const Json::Value& rArr = configJson[n][i]["sprite"]["directions"]["right"];
      for (auto i = 0; i < uArr.size(); i++)
        m[tileObject::UP][i] = &sprites[uArr[i].asString()];
      for (auto i = 0; i < dArr.size(); i++)
        m[tileObject::DOWN][i] = &sprites[dArr[i].asString()];
      for (auto i = 0; i < lArr.size(); i++)
        m[tileObject::LEFT][i] = &sprites[lArr[i].asString()];
      for (auto i = 0; i < rArr.size(); i++)
        m[tileObject::RIGHT][i] = &sprites[rArr[i].asString()];
    }
  }
  return m;
};

ConfigurationController::ConfigurationController (std::string configFilePath, std::map<std::string, Sprite> s)
{

  sprites = s;
  std::ifstream configFile(configFilePath.c_str());
  configFile >> configJson;


  ////////////////
  //  GENERAL
  ///////////////
  chunkFuzz = configJson["map"]["chunks"]["fuzz"].asInt();
  gameSize = configJson["gameSize"].asInt();
  tileSize = configJson["tileSize"].asInt();
  spriteSize = configJson["spriteSize"].asInt();

  ////////////////
  //  TERRAINS
  ///////////////
  for (auto i = 0; i < configJson["terrains"].size(); ++i)
  {
    animationMap aMap = configureAnimationMap(i, "terrains");
    std::string tileTypeName = configJson["terrains"][i]["name"].asString();
    bool impassable = configJson["terrains"][i]["impassable"].asBool();
    bool clusters = configJson["terrains"][i]["clusters"].asBool();
    float multiplier = configJson["terrains"][i]["multiplier"].asFloat();
    float objectFrequencyMultiplier = configJson["terrains"][i]["objectFrequencyMultiplier"].asFloat();
    std::vector<std::string> relatedObjectTypes;
    std::vector<std::string> relatedObjectTypeProbabilities;
    const Json::Value& relatedObjectsArray = configJson["terrains"][i]["objects"];
    for (int i = 0; i < relatedObjectsArray.size(); i++)
    {
      if (relatedObjectsArray[i].isString())
      {
        relatedObjectTypes.push_back(relatedObjectsArray[i].asString());
        for (auto a = 0; a < 10; a++)
          relatedObjectTypeProbabilities.push_back(relatedObjectsArray[i].asString());
      }
      else if (relatedObjectsArray[i].isObject())
      {
        std::string objectTypeName = relatedObjectsArray[i]["type"].asString();
        float objectTypeNameFrequency = relatedObjectsArray[i]["multiplier"].asFloat();
        relatedObjectTypes.push_back(objectTypeName);
        for (auto a = 0; a < 10 * objectTypeNameFrequency; a++)
          relatedObjectTypeProbabilities.push_back(objectTypeName);
      }
    }
    TileType tileType { tileTypeName };
    TerrainType terrainType {
      tileTypeName,
      relatedObjectTypes,
      objectFrequencyMultiplier,
      relatedObjectTypeProbabilities,
      impassable,
      multiplier,
      clusters
    };
    terrainType.animationMap = aMap;
    if (aMap[tileObject::DOWN].size() > 1)
    {
      terrainType.animationSpeed = 1000;
    }
    tileTypes[tileType.name] = tileType;
    terrainTypes[terrainType.name] = terrainType;
    terrainTypesKeys.push_back(terrainType.name);
    SDL_Log("- Loaded '%s' terrain", tileTypeName.c_str());
  }

  ////////////////
  //  BIOMES
  ///////////////
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

    for (auto i = b.maxDepth; i >= b.minDepth; i--)
    {
      biomeLevelMap[i][b.name] = &biomeTypes[b.name];
      for (int j = 0; j < 10 * b.multiplier; j++)
        biomeTypeProbabilities[i].push_back(b.name);
    }

    biomeTypeKeys.push_back(b.name);
    SDL_Log("- Loaded '%s' biome", b.name.c_str());
  }

  ////////////////////
  //  WORLDOBJECTS
  ///////////////////
  for (auto i = 0; i < configJson["objects"].size(); ++i)
  {
    std::string objectTypeName = configJson["objects"][i]["name"].asString();
    bool impassable = configJson["objects"][i]["impassable"].asBool();
    bool clusters = configJson["objects"][i]["clusters"].asBool();
    const Json::Value& biomesArray = configJson["objects"][i]["biomes"];
    std::map<std::string, int> bM;
    for (int i = 0; i < biomesArray.size(); i++)
    {
      bM[biomesArray[i].asString()] = 1;
    }

    animationMap aMap = configureAnimationMap(i, "objects");;
    auto o = ObjectType( 
      objectTypeName,
      impassable,
      configJson["objects"][i]["multiplier"].asFloat(),
      clusters,
      aMap,
      1000,
      bM
    );
    objectTypes[objectTypeName] = o;
    SDL_Log("- Loaded '%s' object", objectTypeName.c_str());
  }

  ////////////////
  //  MOBS
  ///////////////
  for (auto i = 0; i < configJson["mobs"].size(); ++i)
  {
    std::string mobTypeName = configJson["mobs"][i]["name"].asString();
    const Json::Value& biomesArray = configJson["mobs"][i]["biomes"];
    std::map<std::string, int> bM;
    for (int i = 0; i < biomesArray.size(); i++)
    {
      bM[biomesArray[i].asString()] = 1;
    }
    SDL_Log("- Loaded '%s' mob", mobTypeName.c_str());
    animationMap aMap = configureAnimationMap(i, "mobs");
    MobType mobType { 
      mobTypeName,
      false,
      configJson["mobs"][i]["multiplier"].asFloat(),
      0,
      aMap,
      1000,
      bM
    };
    mobTypes[mobType.name] = mobType;
  };
}