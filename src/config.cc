#include "config.h"

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
    std::map<int, std::map<int, Sprite*>> animationMap;
    std::string spriteName;
    if (configJson["terrains"][i]["sprite"].isString())
    {
      spriteName = configJson["terrains"][i]["sprite"].asString();
      animationMap[0][0] = &sprites[spriteName];
    }
    else if (configJson["terrains"][i]["sprite"].isArray())
    {
      const Json::Value& animationArr = configJson["terrains"][i]["sprite"];
      spriteName = animationArr[0].asString();
      for (int i = 0; i < animationArr.size(); i++)
      {
        animationMap[0][i] = &sprites[animationArr[i].asString()];
      }
    }
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
    TileType tileType { &sprites[spriteName], tileTypeName };
    TerrainType terrainType { &sprites[spriteName], tileTypeName, relatedObjectTypes };
    terrainType.impassable = impassable;
    terrainType.multiplier = multiplier;
    terrainType.clusters = clusters;
    terrainType.objectFrequencyMultiplier = objectFrequencyMultiplier;
    terrainType.objectTypeProbabilities = relatedObjectTypeProbabilities;
    terrainType.animationMap = animationMap;
    if (animationMap[0].size() > 1)
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

    std::map<int, std::map<int, Sprite*>> animationMap; // TODO : Fix this
    std::string spriteName;
    if (configJson["objects"][i]["sprite"].isString())
    {
      spriteName = configJson["objects"][i]["sprite"].asString();
      animationMap[tileObject::DOWN][0] = &sprites[spriteName];
    }
    else if (configJson["objects"][i]["sprite"].isArray())
    {
      const Json::Value& animationArr = configJson["objects"][i]["sprite"];
      spriteName = animationArr[0].asString();
      for (int i = 0; i < animationArr.size(); i++)
      {
        animationMap[tileObject::DOWN][i] = &sprites[animationArr[i].asString()];
      }
    }
    ObjectType o { &sprites[spriteName], objectTypeName, impassable, configJson["objects"][i]["multiplier"].asFloat(), clusters, animationMap, 1000, bM };
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
    std::map<int, std::map<int, Sprite*>> animationMap; // TODO : Fix this
    std::string spriteName;
    if (configJson["mobs"][i]["sprite"].isString())
    {
      spriteName = configJson["mobs"][i]["sprite"].asString();
      animationMap[tileObject::DOWN][0] = &sprites[spriteName];
    }
    else if (configJson["mobs"][i]["sprite"].isArray())
    {
      const Json::Value& animationArr = configJson["mobs"][i]["sprite"];
      spriteName = animationArr[0].asString();
      for (int i = 0; i < animationArr.size(); i++)
      {
        animationMap[tileObject::DOWN][i] = &sprites[animationArr[i].asString()];
      }
    }
    else if (configJson["mobs"][i]["sprite"].isObject())
    {
      if (configJson["mobs"][i]["sprite"]["directions"].isObject())
      {
        const Json::Value& uArr = configJson["mobs"][i]["sprite"]["directions"]["up"];
        const Json::Value& dArr = configJson["mobs"][i]["sprite"]["directions"]["down"];
        const Json::Value& lArr = configJson["mobs"][i]["sprite"]["directions"]["left"];
        const Json::Value& rArr = configJson["mobs"][i]["sprite"]["directions"]["right"];
        for (auto i = 0; i < uArr.size(); i++)
          animationMap[tileObject::UP][i] = &sprites[uArr[i].asString()];
        for (auto i = 0; i < dArr.size(); i++)
          animationMap[tileObject::DOWN][i] = &sprites[dArr[i].asString()];
        for (auto i = 0; i < lArr.size(); i++)
          animationMap[tileObject::LEFT][i] = &sprites[lArr[i].asString()];
        for (auto i = 0; i < rArr.size(); i++)
          animationMap[tileObject::RIGHT][i] = &sprites[rArr[i].asString()];
      }
    }
    MobType mobType { &sprites[spriteName], mobTypeName, false, configJson["mobs"][i]["multiplier"].asFloat(), 0, animationMap, 1000, bM };
    mobTypes[mobType.name] = mobType;
  };
}