#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include "sprite.h"
#include "timer.h"
#include "uuid.h"
#include "type/type.h"
#include "object/object.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

// TODO: Switch from std::rand to <random>

struct Player {
  int x;
  int y;
  TileType* tileType;
};

namespace objects
{
  typedef std::map<std::string, MobType> mobTypesMap;
  typedef std::map<std::string, ObjectType> objectTypesMap;
  typedef std::map<std::string, BiomeType> biomeTypesMap;
  typedef std::map<std::string, TerrainType> terrainTypesMap;
  typedef std::map<std::string, TileType> tileTypesMap;
  typedef std::vector<std::shared_ptr<WorldObject>> objectsVector;
  typedef std::map<int, std::map<std::pair<int, int>, BiomeObject>> biomeMap;
  typedef std::map<int, std::map<std::pair<int, int>, TerrainObject>> terrainMap;
  typedef std::map<int, std::map<std::pair<int, int>, std::vector<std::shared_ptr<WorldObject>>>> worldMap;
  typedef std::map<int, std::map<std::pair<int, int>, std::vector<std::shared_ptr<MobObject>>>> mobMap;
}

#endif
