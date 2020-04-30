#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include "sprite.h"
#include "timer.h"
#include "uuid.h"

#include "type/generic.h"
#include "type/terrain.h"
#include "type/object.h"
#include "type/mob.h"
#include "type/tile.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <tuple>

#include <random>

// TODO: Switch from std::rand to <random>

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

struct BiomeObject
{
  int x;
  int y;
  BiomeType* biomeType;
};

struct Tile
{
  int x;
  int y;
  BiomeType* biomeType;
  TerrainType* terrainType;
  Sprite* sprite;
  Timer animationTimer;
  int animationFrame;
  int animationSpeed;
  bool isAnimated() { return animationSpeed > 0; }
  bool seen;
  bool initialized;
  Tile() : seen(false), initialized(false) {}
  Tile (int x, int y) { this->x = x; this->y = y; }
};

struct TerrainObject : Tile
{
  TerrainType* terrainType;
  TerrainObject () {}
  TerrainObject (int x, int y, BiomeType* b, TerrainType* t)
  {
    this->x = x;
    this->y = y;
    biomeType = b;
    terrainType = t;
    sprite = t->sprite;
  }
};

struct WorldObject : Tile
{
  ObjectType* objectType;
  WorldObject() {}
  WorldObject(int x, int y, ObjectType* o, BiomeType* b)
  {
    this->x = x;
    this->y = y;
    objectType = o;
    biomeType = b;
    sprite = o->sprite;
  }
};


struct SimulatedObject : Tile
{
  std::map<std::string, Timer*> objectTimers;
  bool dead;
  SimulatedObject () : dead(false) {}
  void kill()
  {
    dead = true;
  }
  void initSimulation()
  {
    Timer t;
    t.start();
    objectTimers["lifetime"] = &t;
  }
};


struct MobObject : SimulatedObject
{
  std::string id;
  int speed;
  MobType* mobType;
  std::map<std::string, Timer> mobTimers;
  MobObject (int x, int y, MobType* m, BiomeType* b)
  {
    id = uuid::generate_uuid_v4();
    this->initSimulation();
    this->x = x;
    this->y = y;
    mobType = m;
    biomeType = b;
    sprite = m->sprite;
    speed = std::rand() % 1500 + 1000;
    Timer t;
    t.start();
    mobTimers["movement"] = t;
  }
};

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
