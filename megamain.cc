#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "json/json.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>


////////////////////
//  PRIMITIVES
///////////////////

struct Image
{
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite
{
  int tileMapX;
  int tileMapY;
  std::string name;
};

struct Timer {
  int last;
  int current;
  bool paused;
  bool started;
  void start()
  {
    started = true;
    paused = false;
    current = 0;
    last = SDL_GetTicks();
  }
  void stop()
  {
    started = false;
    paused = false;
    last = 0;
    current = 0;
  }
  void reset() {
    stop();
    start();
  }
  void pause()
  {
    if (started && !paused)
    {
      paused = true;
      current = SDL_GetTicks() - last;
      last = 0;
    }
  }
  void unpause()
  {
    if (started && paused) {
      paused = false;
      last = SDL_GetTicks() - current;
      current = 0;
    }
  }
  int elapsed()
  {
    auto time = 0;
    if (started)
    {
      if (paused)
      {
        time = current;
      }
      else
      {
        time = SDL_GetTicks() - last;
      }
    }
    return time;
  }
  Timer () : last(0), current(0), paused(false), started(false) {}
};

struct Rect
{
  int x1;
  int y1;
  int x2;
  int y2;
  std::vector<Rect> rects;
  Rect () {}
  Rect (int a, int b, int c, int d) { x1 = a; y1 = b; x2 = c; y2 = d; }
  void set(std::tuple<int, int, int, int> data) { auto [_x1, _y1, _x2, _y2] = data; x1 = _x1; y1 =_y1; x2 =_x2; y2 = _y2; }
  std::tuple<int, int, int, int> get(){ return std::make_tuple(x1, y1, x2, y2 ); };
  SDL_Rect* getSDL_Rect () { auto r = new SDL_Rect(); r->x = x1; r->y = y1; r->w = x2; r->h = y2; return r; }
  int getWidth () { return std::abs(x1) + std::abs(x2); }
  int getHeight () { return std::abs(y1) + std::abs(y2); }
  std::pair<int, int> getDimensions () { return { getWidth(), getHeight() }; }
  std::tuple<int, int> getMid (){ return std::make_tuple(std::floor((x1 + x2)/2), std::floor((y1 + y2)/2)); }
  std::vector<Rect>* getRects(bool shuffle = false) // TODO: Don't clear every time, create different method
  {
    int small_w = 25;
    int small_h = 25;
    int w = getWidth();
    int h = getHeight();
    auto result_w = std::div(w, small_w);
    auto result_h = std::div(h, small_h);
    rects.clear();
    for (auto x = x1; x <= x2; x += result_w.quot)
      for (auto y = y1; y <= y2; y += result_h.quot)
      {
        Rect r { x, y, x + result_w.quot, y + result_h.quot };
        rects.push_back(r);
      }
    if (shuffle == true)
    {
      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::default_random_engine rng(seed);
      std::shuffle(rects.begin(), rects.end(), rng);
    }
    return &rects;
  }
  std::vector<Rect>* getShuffledRects() { return getRects(true); }
  void multiprocess(std::function<void(int, int)> f, Rect* r = NULL, int fuzz = 1)
  {
    if (r == NULL)
      r->set({ x1, y1, x2, y2 });
    for (auto i = x1; i < x2; i += 1 + std::rand() % fuzz)
      for (auto j = y1; j < y2; j += 1 + std::rand() % fuzz)
      {
        std::thread t([&f, i, j]() { f(i, j); });
        t.join();
      }
  }
};

////////////////////
//  UUID
///////////////////

namespace uuid
{
  std::string generate_uuid_v4();
}

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dis(0, 15);
static std::uniform_int_distribution<> dis2(8, 11);

std::string uuid::generate_uuid_v4()
{
  std::stringstream ss;
  int i;
  ss << std::hex;
  for (i = 0; i < 8; i++)
    ss << dis(gen);
  ss << "-";
  for (i = 0; i < 4; i++)
    ss << dis(gen);
  ss << "-4";
  for (i = 0; i < 3; i++)
    ss << dis(gen);
  ss << "-";
  ss << dis2(gen);
  for (i = 0; i < 3; i++)
    ss << dis(gen);
  ss << "-";
  for (i = 0; i < 12; i++)
    ss << dis(gen);
  return ss.str();
}

////////////////////
//  INPUT
///////////////////

namespace input
{
  enum directions
  {
    UP        = 0x01,
    DOWN      = 0x02,
    LEFT      = 0x04,
    RIGHT     = 0x08
  };
  struct UserInputHandler
  {
    SDL_Event appEvent;
    Timer lock;
    UserInputHandler()
    {
      lock.start();
    }
    void handleKeyboardMovement (std::function<void(int)>);
    void handleAppEvents (std::function<void(SDL_Event*)>);
  };
}

void input::UserInputHandler::handleKeyboardMovement (std::function<void(int)> f)
{
  auto *ks = SDL_GetKeyboardState(NULL);
  if (ks[SDL_SCANCODE_ESCAPE] && ks[SDL_SCANCODE_LCTRL])
  {
    f(-1);
  }
  while(ks[SDL_SCANCODE_LEFT]
      || ks[SDL_SCANCODE_RIGHT]
      || ks[SDL_SCANCODE_UP]
      || ks[SDL_SCANCODE_DOWN]
    )
  {
    if ((ks[SDL_SCANCODE_DOWN] && ks[SDL_SCANCODE_UP])
        || (ks[SDL_SCANCODE_LEFT] && ks[SDL_SCANCODE_RIGHT])
      )
      break;
    int directions = 0x00;
    if (ks[SDL_SCANCODE_LEFT])
      directions += input::LEFT;
    if (ks[SDL_SCANCODE_RIGHT])
      directions += input::RIGHT;
    if (ks[SDL_SCANCODE_UP])
      directions += input::UP;
    if (ks[SDL_SCANCODE_DOWN])
      directions += input::DOWN;
    f(directions);
    SDL_PumpEvents();
  }
};

void input::UserInputHandler::handleAppEvents (std::function<void(SDL_Event*)> f)
{
  SDL_PollEvent(&appEvent);
  f(&appEvent);
};

////////////////////
//  TYPES
///////////////////

struct GenericType
{
  std::string name;
  bool impassable;
  float multiplier;
  bool clusters;
  std::map<int, std::map<int, Sprite*>> animationMap;
  int animationSpeed;
  int maxFrames(int direction = 0x02) { return animationMap[direction].size(); }
  Sprite* getFrame(int n, int direction = 0x02) { return animationMap[direction][n]; }
  bool isAnimated() { return animationSpeed > 0; }
  float getMultiplier() { if (multiplier > 0) return multiplier; else return 1; }
  GenericType() {};
  GenericType(
    std::string name,
    bool impassable,
    float multiplier,
    bool clusters,
    std::map<int, std::map<int, Sprite*>> animationMap,
    int animationSpeed
  )
  {
    this->name = name;
    this->impassable = impassable;
    this->multiplier = multiplier;
    this->clusters = clusters;
    this->animationMap = animationMap;
    this->animationSpeed = animationSpeed;
  }
};

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

struct TileType : GenericType
{
  TileType () {}
  TileType (std::string n) { this->name = n; }
};

struct ObjectType : GenericType
{
  std::map<std::string, int> biomes;
  ObjectType() {};
  ObjectType(
    std::string name,
    bool impassable,
    float multiplier,
    bool clusters,
    std::map<int, std::map<int, Sprite*>> animationMap,
    int animationSpeed,
    std::map<std::string, int> biomes
  )
  {
    this->name = name;
    this->impassable = impassable;
    this->multiplier = multiplier;
    this->clusters = clusters;
    this->animationMap = animationMap;
    this->animationSpeed = animationSpeed;
    this->biomes = biomes;
  }
  bool canExistIn(std::string biomeName) { return biomes[biomeName] == 1; }
};

struct MobType : ObjectType
{
  MobType() {};
  MobType(
    std::string name,
    bool impassable,
    float multiplier,
    bool clusters,
    std::map<int, std::map<int, Sprite*>> animationMap,
    int animationSpeed,
    std::map<std::string, int> biomes
  )
  {
    this->name = name;
    this->impassable = impassable;
    this->multiplier = multiplier;
    this->clusters = clusters;
    this->animationMap = animationMap;
    this->animationSpeed = animationSpeed;
    this->biomes = biomes;
  }
};

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

////////////////////
//  OBJECTS
///////////////////

namespace tileObject
{
  enum directions
  {
    UP        = 0x01,
    DOWN      = 0x02,
    LEFT      = 0x04,
    RIGHT     = 0x08
  };
  enum types
  {
    BIOME       = 0x01,
    MOB         = 0x02,
    WORLD       = 0x04,
    SIMULATED   = 0x08,
    TERRAIN     = 0x16,
    TILE        = 0x32
  };
}

struct Tile
{
  int z;
  int x;
  int y;
  BiomeType* biomeType;
  TerrainType* terrainType;
  Timer animationTimer;
  int animationFrame;
  int animationSpeed;
  bool isAnimated() { return animationSpeed > 0; }
  bool initialized;
  int type;
  int direction;
  int relativeX;
  int relativeY;
  int drawX;
  int drawY;
  Tile() : initialized(false), direction(tileObject::DOWN) { type = tileObject::TILE; }
  Tile (int x, int y, int z) { this->x = x; this->y = y; this->z = z; type = tileObject::TILE; }
  std::tuple<int, int, int, int> getPosition() { return std::make_tuple(z, x, y, direction); }
  void setPosition(std::tuple<int, int, int> position)
  {
    z = std::get<0>(position);
    x = std::get<1>(position);
    y = std::get<2>(position);
  }
};

struct TerrainObject : Tile
{
  TerrainType* terrainType;
  TerrainObject () { type = tileObject::TERRAIN; }
  TerrainObject (int x, int y, int z, BiomeType* b, TerrainType* t)
  {
    type = tileObject::TERRAIN;
    this->x = x;
    this->y = y;
    this->z = z;
    biomeType = b;
    terrainType = t;
  }
};

struct WorldObject : Tile
{
  ObjectType* objectType;
  WorldObject() { type = tileObject::WORLD; }
  WorldObject(int x, int y, int z, ObjectType* o, BiomeType* b)
  {
    type = tileObject::WORLD;
    this->x = x;
    this->y = y;
    this->z = z;
    objectType = o;
    biomeType = b;
  }
};

struct BiomeObject
{
  int x;
  int y;
  BiomeType* biomeType;
  int type;
  BiomeObject () { type = tileObject::BIOME; }
};

////////////////////
//  SIMULATOR
///////////////////

namespace simulated
{
  enum actions
  {
    MOVE      = 0x01,
    DIE       = 0x02,
    DELETE    = 0x04
  };
  template <class T>
  class Simulator
  {
    private:
      std::string _id;
      Timer _timer;
      int _frequency;
      int _variation;
      std::function<void()> _fn;
    public:
      Simulator () {}
      Simulator (std::function<void()> fn)
      {
        _fn = fn;
        _id = uuid::generate_uuid_v4();
        _timer.start();
        _frequency = 3000 + std::rand() % 1000;
      }
      void simulate ()
      {
        auto elapsed = _timer.elapsed();
        if (elapsed > _frequency)
        {
          _fn();
          _timer.reset();
        }
      }
  };
}

struct SimulatedObject : Tile
{
  std::map<std::string, Timer*> objectTimers;
  bool dead;
  int orders;
  int status;
  SimulatedObject () : dead(false) { type = tileObject::SIMULATED; }
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
  std::vector<std::shared_ptr<simulated::Simulator<MobObject>>> simulators;
  MobObject (int x, int y, int z, MobType* m, BiomeType* b)
  {
    type = tileObject::MOB;
    id = uuid::generate_uuid_v4();
    this->initSimulation();
    this->x = x;
    this->y = y;
    this->z = z;
    mobType = m;
    biomeType = b;
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

////////////////////
//  CONFIG
///////////////////

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

config::animationMap config::ConfigurationController::configureAnimationMap (int i, std::string n)
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

config::ConfigurationController::ConfigurationController (std::string configFilePath, std::map<std::string, Sprite> s)
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

////////////////////
//  CHUNK
///////////////////

namespace map::chunk
{
  struct ChunkProcessor;
  typedef std::function<void(int, int, int)> chunkFunctor;
  typedef std::function<void(int, int, int, BiomeType*)> chunkProcessorFunctor;
  typedef std::function<void(Rect*, BiomeType* b)> chunkProcessorCallbackFunctor;
  typedef std::function<BiomeType*(ChunkProcessor*,int,std::tuple<int,int>)> chunkCallbackFn;
  typedef std::variant<chunkFunctor, chunkProcessorFunctor, chunkProcessorCallbackFunctor> genericChunkFunctor;
  typedef std::vector<std::pair<genericChunkFunctor, chunkCallbackFn>> multiprocessFunctorVec;
  typedef std::array<multiprocessFunctorVec, 2> multiprocessFunctorArray;

  struct ChunkReport
  {
    std::map<int, std::map<std::string, int >> terrainCounts;
    std::map<int, std::map<std::string, int >> biomeCounts;
    std::map<int, std::tuple<int, std::string>> topTerrain;
    std::map<int, std::tuple<int, std::string>> topBiome;
    std::map<int, std::map<std::string, std::string>> meta;
  };

  struct ChunkProcessor
  {
    Rect* chunk;
    std::vector<Rect>* smallchunks;
    int zMax;
    BiomeType* brush;
    std::shared_mutex brushMtx;
    ChunkProcessor (Rect* r, int zMax = 3)
    {
      chunk = r;
      bool shuffle = true;
      smallchunks = r->getRects(shuffle);
      this->zMax = zMax;
    };
    BiomeType* getBrush() { std::shared_lock lock(brushMtx); return brush; }
    void setBrush(BiomeType* b)
    {
      std::unique_lock lock(brushMtx);
      brush = b;
    }
    void processEdges(Rect*, std::pair<chunkProcessorFunctor, BiomeType*>);
    void multiProcess (Rect*, multiprocessFunctorArray, int);
    void lazyProcess (Rect* r, std::vector<chunkFunctor>, int);
    void process (Rect*, std::vector<chunkFunctor>);
    void processChunk (std::vector<chunkFunctor> functors) { process(chunk, functors); }
    void lazyProcessChunk (std::vector<chunkFunctor> functors, int fuzz = 1) { lazyProcess(chunk, functors, fuzz); }
    void multiProcessChunk (multiprocessFunctorArray functors, int fuzz = 1) { multiProcess(chunk, functors, fuzz); }
  };

  ChunkReport getRangeReport(std::function<void(int, int, ChunkReport*)>, Rect*, int, int);
}

void map::chunk::ChunkProcessor::processEdges(Rect* r, std::pair<chunkProcessorFunctor, BiomeType*> f)
{
  Rect top { chunk->x1, chunk->y1, chunk->x2, chunk->y1 };
  Rect bottom { chunk->x1, chunk->y2, chunk->x2, chunk->y2 };
  Rect left { chunk->x1, chunk->y1, chunk->x1, chunk->y2 };
  Rect right { chunk->x2, chunk->y1, chunk->x2, chunk->y2 };
  std::vector<Rect> edges { top, bottom, left, right };
  for (auto it = edges.begin(); it != edges.end(); ++it)
  {
    std::thread t([this, f](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < zMax; h++)
        for (auto i = x1; i <= x2; i++)
          for (auto j = y1; j <= y2; j++)
            f.first(h, i, j, f.second);
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
  }
}

void map::chunk::ChunkProcessor::multiProcess (Rect* r, multiprocessFunctorArray functors, int fuzz = 1)
{

  // Process every tile in every chunk
  for (auto it = smallchunks->begin(); it != smallchunks->end(); ++it)
  {
    for (auto f : functors[0])
    {
      BiomeType* b = f.second(this, 0, it->getMid());
      auto fn = std::get<chunkProcessorFunctor>(f.first);
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Generating '%s' chunk", b->name.c_str());
      int n = std::rand() % 10;
      std::thread t([this, n, fn, b, fuzz](int x1, int y1, int x2, int y2) {
        for (auto h = 0; h < zMax; h += std::rand() % fuzz + 1)
          for (auto i = n > 5 ? x1 : x2; [i,x2,x1,n](){if(n>5)return i<=x2;else return i>=x1;}() ; [&i,n, fuzz](){if(n>5)i+=std::rand()%fuzz+1;else i-=(std::rand()%fuzz+1);}())
            for (auto j = n > 5 ? y1 : y2; [j,y2,y1,n](){if(n>5)return j<=y2;else return j>=y1;}() ; [&j,n, fuzz](){if(n>5)j+=std::rand()%fuzz+1;else j-=(std::rand()%fuzz+1);}())
              fn(h, i, j, b);
      }, it->x1, it->y1, it->x2, it->y2);
      t.join();
    }
    // TODO: This may be unnecessary
    // TODO: Re-think map processing flows
    // Post-process chunk thread
    for (auto f : functors[1])
    {
      BiomeType* b = f.second(this, 0, it->getMid());
      auto fn = std::get<chunkProcessorCallbackFunctor>(f.first);
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Post-processing chunk (%d, %d)", it->x1, it->x2);
      std::thread t([fn, &b, it]() {
        fn(&(*it), b);
      });
      t.join();
    }
  }
  
}
void map::chunk::ChunkProcessor::lazyProcess (Rect* r, std::vector<chunkFunctor> functors, int fuzz = 1)
{
  for (auto h = 0; h < zMax; h += std::rand() % fuzz + 1)
    for (auto i = r->x1; i < r->x2; i += std::rand() % fuzz + 1)
      for (auto j = r->y1; j < r->y2; j += std::rand() % fuzz + 1)
        for (auto f : functors) f(h, i, j);
}
void map::chunk::ChunkProcessor::process (Rect* r, std::vector<chunkFunctor> functors)
{
  for (auto h = 0; h < zMax; h++)
    for (auto i = r->x1; i != r->x2; i++)
      for (auto j = r->y1; j != r->y2; j++)
        for (auto f : functors) f(h, i, j);
}

map::chunk::ChunkReport map::chunk::getRangeReport(std::function<void(int, int, map::chunk::ChunkReport*)> f, Rect* r, int fuzz = 1, int base = 1)
{
  ChunkReport report;
  for (auto i = r->x1; i < r->x2; i += base + std::rand() % fuzz)
    for (auto j = r->y1; j < r->y2; j += base + std::rand() % fuzz)
      f(i, j, &report);
  return report;
}

////////////////////
//  MAP
///////////////////

namespace map
{
  struct MapGenerator
  {
    bool processing;
    MapGenerator () : processing(false){}
    void init(std::shared_mutex* mtx)
    {
      mtx->lock();
      processing = true;
      mtx->unlock();
    }
    void reset(std::shared_mutex* mtx)
    {
      mtx->lock();
      processing = false;
      mtx->unlock();
    }
  };

  struct MapController
  {
    MapGenerator mapGenerator;
    int maxDepth;
    objects::mobTypesMap* mobTypes;
    objects::objectTypesMap* objectTypes;
    std::map<std::string, BiomeType>* biomeTypes;
    std::vector<std::string>* biomeTypeKeys;
    objects::terrainTypesMap* terrainTypes;
    objects::tileTypesMap* tileTypes;
    objects::biomeMap biomeMap;
    objects::terrainMap terrainMap;
    objects::worldMap worldMap;
    objects::mobMap mobMap;
    config::ConfigurationController* cfg;
    MapController () : maxDepth(0) {}
    MapController (
        int d,
        objects::mobTypesMap* mTypes,
        objects::objectTypesMap* oTypes,
        objects::biomeTypesMap* bTypes,
        std::vector<std::string>* bTypeKeys,
        objects::terrainTypesMap* tnTypes,
        objects::tileTypesMap* tlTypes,
        config::ConfigurationController* c
    )
    {
      maxDepth = d; mobTypes = mTypes; objectTypes = oTypes; biomeTypes = bTypes; biomeTypeKeys = bTypeKeys;
      terrainTypes = tnTypes; tileTypes = tlTypes; cfg = c;
    }
    bool isPassable (std::tuple<int, int, int>);
    BiomeType* updateTile (int, int, int, BiomeType*, TerrainType*, std::vector<std::shared_ptr<WorldObject>>);
    void updateTile (int, int, int, std::shared_ptr<WorldObject>, std::shared_ptr<MobObject>);
    std::vector<std::shared_ptr<MobObject>>::iterator moveMob (std::string, std::tuple<int, int, int>, std::tuple<int, int, int>);
    std::vector<std::shared_ptr<MobObject>>::iterator moveMob (std::vector<std::shared_ptr<MobObject>>::iterator, std::tuple<int, int, int>, int directions);
    std::map<int, std::map<std::string, int>> getTilesInRange (Rect*);
    std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (Rect*);
    std::map<int, std::map<std::string, int>> getBiomesInRange (Rect* rangeRect);
    map::chunk::ChunkReport generateRangeReport(Rect*, int);
    void processChunk(Rect*, std::function<void(int, int, int)>);
    template<typename F> void iterateOverChunk(Rect*, F);
    template<typename F> void iterateOverChunkEdges(Rect*, F);
    void randomlyAccessAllTilesInChunk(Rect*, std::function<void(int, int, int)>);
    std::map<int, std::vector<SDL_Point>> getAllPointsInRect(Rect*);
    int generateMapChunk(Rect*);
  };
}

std::shared_mutex mtx;

std::map<int, std::vector<SDL_Point>> map::MapController::getAllPointsInRect(Rect* r)
{
  std::map<int, std::vector<SDL_Point>> results;
  for (auto h = 0; h < maxDepth; h++)
  {
    std::vector<SDL_Point> points;
    for (auto i = r->x1; i != r->x2; i++)
    {
      for (auto j = r->y1; j != r->y2; j++)
      {
        SDL_Point p = { i, j };
        points.push_back(p);
      }
    }
    results[h] = points;
  }
  return results;
}


std::map<int, std::map<std::string, int>> map::MapController::getTilesInRange (Rect* r)
{
  std::map<int, std::map<std::string, int>> t;
  auto lambda = [this, &t](int h, int i, int j)
  {
    if (terrainMap[h].find({ i, j }) != terrainMap[h].end())
      t[h][terrainMap[h][{i, j}].terrainType->name] += 1;
  };
  processChunk(r, lambda);
  return t;
}


std::map<int, std::map<std::string, std::map<std::string, int>>> map::MapController::getCountsInRange (Rect* r)
{
  std::map<int, std::map<std::string, std::map<std::string, int>>> res;
  auto lambda = [this, &res](int h, int i, int j)
  {
    auto it = terrainMap[h].find({ i, j });
    if (it != terrainMap[h].end())
    {
      res[h]["terrain"][it->second.terrainType->name]++;
      res[h]["biome"][it->second.biomeType->name]++;
    }
  };
  processChunk(r, lambda);
  return res;
}



std::map<int, std::map<std::string, int>> map::MapController::getBiomesInRange (Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> results;
  auto lambda = [this, &results](int h, int i, int j)
  {
    if (terrainMap[h].find({ i, j }) != terrainMap[h].end())
      results[h][terrainMap[h][{i, j}].biomeType->name] += 1;
  };
  processChunk(rangeRect, lambda);
  return results;
}


map::chunk::ChunkReport map::MapController::generateRangeReport(Rect* range, int h = 0)
{
  std::unique_lock lock(mtx);
  auto t = map::chunk::getRangeReport([this, h](int x, int y, map::chunk::ChunkReport* r){
    auto it = terrainMap[h].find({x, y});
    if (it != terrainMap[h].end())
    {
      r->terrainCounts[h][it->second.terrainType->name]++;
      auto [ topTerrainCount, topTerrainName ] = r->topTerrain[h];
      if (r->terrainCounts[h][it->second.terrainType->name] > topTerrainCount)
      {
        r->meta[h]["secondTopTerrain"] = topTerrainName;
        r->topTerrain[h] = { r->terrainCounts[h][it->second.terrainType->name], it->second.terrainType->name };
      }
      if (cfg->biomeExistsOnLevel(it->second.biomeType->name, h) == true)
      {
        r->biomeCounts[h][it->second.biomeType->name]++;
        auto [ topBiomeCount, topBiomeName ] = r->topBiome[h];
        if (r->biomeCounts[h][it->second.biomeType->name] > topBiomeCount)
        {
          r->meta[h]["secondTopBiome"] = topBiomeCount;
          r->topBiome[h] = { r->biomeCounts[h][it->second.biomeType->name], it->second.biomeType->name };
        }
      }
    }
  }, range, 1, 1);
  return t;
}

std::shared_mutex tileMutex;

BiomeType* map::MapController::updateTile (int z, int x, int y, BiomeType* biomeType, TerrainType* terrainType, objects::objectsVector worldObjects = objects::objectsVector ())
{
  if (!cfg->biomeExistsOnLevel(biomeType->name, z))
  {
    biomeType = cfg->getRandomBiomeType(z);
    terrainType = &cfg->terrainTypes[biomeType->getRandomTerrainTypeName()];
  }
  std::unique_lock lock(tileMutex);
  TerrainObject t;
  t.x = x;
  t.y = y;
  t.biomeType = biomeType;
  t.terrainType = terrainType;
  if (terrainType->isAnimated())
  {
    t.animationTimer.start();
    t.animationSpeed = terrainType->animationSpeed + std::rand() % 3000;
  }
  terrainMap[z][{ x, y }] = t;


  for (auto o : worldMap[z][{ x, y }])
    if (!o->objectType->biomes[biomeType->name])
    {
      worldMap[z][{ x, y }].clear();
      break;
    }
  
  
  mobMap[z][{ x, y }].clear();
  BiomeObject b;
  b.biomeType = biomeType;
  b.x = x;
  b.y = y;
  biomeMap[z][{ x, y }] = b;
  return biomeType;
}

void map::MapController::updateTile (int z, int x, int y, std::shared_ptr<WorldObject> w = nullptr, std::shared_ptr<MobObject> m = nullptr)
{
  std::unique_lock lock(tileMutex);
  if (w != nullptr)
  {
    worldMap[z][{x, y}].push_back(w);
  }
  if (m != nullptr)
  {
    mobMap[z][{x, y}].push_back(std::move(m));
  }
}

bool map::MapController::isPassable (std::tuple<int, int, int> coords)
{
  auto [_z, _x, _y] = coords;
  auto terrainIt = terrainMap[_z].find({ _x, _y });
  if (terrainIt == terrainMap[_z].end())
    return false;
  else if (terrainIt->second.terrainType->impassable)
    return false;
  auto worldIt = worldMap[_z].find({ _x, _y });
  if (worldIt != worldMap[_z].end())
    for (auto o : worldIt->second)
      if (o->objectType->impassable)
        return false;
  auto mobIt = mobMap[_z].find({ _x, _y });
  if (mobIt != mobMap[_z].end())
    for (auto o : mobIt->second)
      if (o->mobType->impassable)
        return false;
  return true;
}

std::shared_mutex mobMtx;

std::vector<std::shared_ptr<MobObject>>::iterator
map::MapController::moveMob (std::string id, std::tuple<int, int, int> origin, std::tuple<int, int, int> destination)
{
  auto [z1, x1, y1] = origin;
  auto [z2, x2, y2] = destination;
  if (!isPassable(destination))
    return mobMap[z1][{x1, y1}].begin();
  std::unique_lock lock(mobMtx);
  auto it = mobMap[z1][{x1, y1}].begin();
  while (it != mobMap[z1][{x1, y1}].end())
  {
    if (it->get()->id == id)
    {
      it->get()->setPosition({ z2, x2, y2 });
      mobMap[z2][{x2, y2}].push_back((*it));
      it = mobMap[z1][{x1, y1}].erase(it);
      return it;
    }
    else
    {
      ++it;
    }
  }
  return it;
}

std::vector<std::shared_ptr<MobObject>>::iterator
map::MapController::moveMob (std::vector<std::shared_ptr<MobObject>>::iterator mobIt, std::tuple<int, int, int> coords, int directions)
{
  auto mob = mobIt->get();
  auto [z1, x1, y1] = coords;
  auto [z2, x2, y2] = coords;
  if (directions & tileObject::LEFT)
  {
    if (mob->direction == tileObject::LEFT)
    {
      x2--;
    }
    else
    {
      mob->direction = tileObject::LEFT;
      return ++mobIt;
    }
  }
  if (directions & tileObject::RIGHT)
  {
    if (mob->direction == tileObject::RIGHT)
    {
      x2++;
    }
    else
    {
      mob->direction = tileObject::RIGHT;
      return ++mobIt;
    }
  }
  if (directions & tileObject::UP)
  {
    if (mob->direction == tileObject::UP)
    {
      y2--;
    }
    else
    {
      mob->direction = tileObject::UP;
      return ++mobIt;
    }
  }
  if (directions & tileObject::DOWN)
  {
    if (mob->direction == tileObject::DOWN)
    {
      y2++;
    }
    else
    {
      mob->direction = tileObject::DOWN;
      return ++mobIt;
    }
  }
  SDL_Point offset = {0, 0};
  if (mob->direction == tileObject::RIGHT)
    offset.x -= cfg->tileSize;
  if (mob->direction == tileObject::DOWN)
    offset.y -= cfg->tileSize;
  if (mob->direction == tileObject::UP)
    offset.y += cfg->tileSize;
  if (mob->direction == tileObject::LEFT)
    offset.x += cfg->tileSize;
  mob->relativeX = offset.x;
  mob->relativeY = offset.y;
  return moveMob(mob->id, {z1, x1, y1}, {z2, x2, y2});
}

void map::MapController::processChunk(Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2
  );
  int n = std::rand() % 10;
  for (auto h = 0; h < maxDepth; h++)
    for (auto i = n > 5 ? chunkRect->x1 : chunkRect->x2; [i,chunkRect,n](){if(n>5)return i<=chunkRect->x2;else return i>=chunkRect->x1;}() ; [&i,n](){if(n>5)i+=1;else i-=1;}())
      for (auto j = n > 5 ? chunkRect->y1 : chunkRect->y2; [j,chunkRect,n](){if(n>5)return j<=chunkRect->y2;else return j>=chunkRect->y1;}() ; [&j,n](){if(n>5)j+=1;else j-=1;}())
        f(h, i ,j);
}

template <typename F>
void map::MapController::iterateOverChunk(Rect* chunkRect, F functors)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2
  );

  Rect r { chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2 };
  auto rects = r.getRects();
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Analyzing chunk: (x1: %d, y1: %d) (x2: %d, y2: %d) [%d]", chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2, static_cast<int>(rects->size()));
  auto it = rects->begin();
  while (it != rects->end())
  {
    std::thread t([this, functors](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < maxDepth; h++)
      {
        BiomeType* b = cfg->getRandomBiomeType(h);
        for (auto i = x1; i != x2; i++)
          for (auto j = y1; j != y2; j++)
            for (auto f : functors)
              f(h, i, j, b);
      }
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "\t- (x1: %d, y1: %d) (x2: %d, y2: %d)", it->x1, it->x2, it->y1, it->y2);
    ++it;
  }
}

template <typename F>
void map::MapController::iterateOverChunkEdges(Rect* chunk, F f)
{
  Rect top { chunk->x1, chunk->y1, chunk->x2, chunk->y1 };
  Rect bottom { chunk->x1, chunk->y2, chunk->x2, chunk->y2 };
  Rect left { chunk->x1, chunk->y1, chunk->x1, chunk->y2 };
  Rect right { chunk->x2, chunk->y1, chunk->x2, chunk->y2 };
  std::vector<Rect> edges { top, bottom, left, right };
  for (auto it = edges.begin(); it != edges.end(); ++it)
  {
    std::thread t([this, f](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < maxDepth; h++)
        for (auto i = x1; i <= x2; i++)
          for (auto j = y1; j <= y2; j++)
            f(h, i, j);
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
  }
}

void map::MapController::randomlyAccessAllTilesInChunk(Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2
  );
  auto coordinates = getAllPointsInRect(chunkRect);
  for (auto h = 0; h < maxDepth; h++)
  {
    while (coordinates[h].size())
    {
      int i = std::rand() % coordinates[h].size();
      SDL_Point p = coordinates[h].at(i);
      f(h, p.x, p.y);
      coordinates[h].erase(coordinates[h].begin() + i);
    }
  }
}

int map::MapController::generateMapChunk(Rect* chunkRect)
{
  if (mapGenerator.processing)
  {
    std::unique_lock lock(mtx);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Already processing chunk.");
    return -1;
  }

  mapGenerator.init(&mtx);

  auto createTerrainObjects = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (it == terrainMap[h].end())
    {
      TerrainType* tt;
      Rect range = { i-1, j-1, i+1, j+1 };
      auto t = generateRangeReport(&range, h);
      auto [tCount, topTerrainName] = t.topTerrain[h];
      auto [bCount, topBiomeName] = t.topBiome[h];
      if (topTerrainName.length() > 0 && cfg->terrainTypes[topTerrainName].clusters == true && cfg->biomeExistsOnLevel(topBiomeName, h))
      {
        tt = &cfg->terrainTypes[topTerrainName];
        b = &cfg->biomeTypes[topBiomeName]; 
      }
      else
        tt = &cfg->terrainTypes[b->getRandomTerrainTypeName()];
      b = updateTile(h, i, j, b, tt);
      if ((std::rand() % 10000 > (9500 - ((9500 * tt->getObjectFrequencyMultiplier()) - 9500))) && tt->objectTypeProbabilities.size() > 0)
      {
        std::string n = tt->getRandomObjectTypeName(); // TODO: First check if it's possible, then keep checking until you've got it
        if (cfg->objectTypes[n].biomes[b->name] && worldMap[h][{i, j}].begin() == worldMap[h][{i, j}].end())
        {
          std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
            i, j, h, &cfg->objectTypes[n], &cfg->biomeTypes[b->name]
          );
          if (cfg->objectTypes[n].isAnimated())
          {
            o->animationTimer.start();
            o->animationSpeed = cfg->objectTypes[n].animationSpeed + std::rand() % 3000;
          }
          updateTile(h, i, j, o, nullptr);
        }  
      }
    }
  };


  auto addMobs = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (isPassable({h, i, j}) && it != terrainMap[h].end() && it->second.initialized == false)
    {
      if (std::rand() % 1000 > 975)
      {
        
        for (auto mob = cfg->mobTypes.begin(); mob != cfg->mobTypes.end(); mob++)
        {
          if (mob->second.biomes.find(it->second.biomeType->name) != mob->second.biomes.end())
          {
            std::shared_ptr<MobObject> m = std::make_shared<MobObject>(
              i, j, h, &mob->second, &cfg->biomeTypes[it->second.biomeType->name]
            );

            if (mob->second.isAnimated())
            {

              m->simulators.push_back(std::make_shared<simulated::Simulator<MobObject>>(
                [this,h,i,j,m]()
                {
                  
                  int n = std::rand() % 100;
                  if (n > 50)
                    m->x += std::rand() % 100 > 50 ? 1 : -1;
                  else
                    m->y += std::rand() % 100 > 50 ? 1 : -1;
                  if (isPassable({h, i, j}))
                    m->orders += simulated::MOVE;
                }
              ));

              m->animationTimer.start();
              m->animationSpeed = mob->second.animationSpeed + std::rand() % 3000;
            }

            updateTile(h, i, j, nullptr, m);
          }
        }
      }
    }
    std::unique_lock lock(mtx);
    it->second.initialized = true;
  };

  auto hammerChunk = [this](Rect* r, BiomeType* b)
  {
    auto hammerProcessor = [this](int h, int i, int j)
    {
      auto it = terrainMap[h].find({ i, j });
      if (it != terrainMap[h].end() && it->second.initialized == false)
      {
        Rect range = { i-3, j-3, i+3, j+3 };
        auto t = generateRangeReport(&range, h);
        auto [bCount, topBiomeName] = t.topBiome[h];
        if (std::rand() % 1000 > 985) topBiomeName = cfg->getRandomBiomeType(h)->name;
        if (terrainMap[h].find({ i, j })->second.initialized == false) updateTile(h, i, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i+1, j })->second.initialized == false) updateTile(h, i+1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i-1, j })->second.initialized == false) updateTile(h, i-1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i, j+1 })->second.initialized == false) updateTile(h, i, j+1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i, j-1 })->second.initialized == false) updateTile(h, i, j-1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
      }
    };
    processChunk(r, hammerProcessor);
  };

  auto cleanChunk = [this](Rect* r, BiomeType* b)
  {
    auto processor = [this](int h, int i, int j)
    {
      auto it = terrainMap[h].find({ i, j });
      if (it != terrainMap[h].end() && it->second.initialized == false)
      {
        Rect range = { i-3, j-3, i+3, j+3 };
        auto t = generateRangeReport(&range, h);
        auto [bCount, topBiomeName] = t.topBiome[h];
        if (t.biomeCounts[h][it->second.biomeType->name] <= 2) updateTile(h, i, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
      }
    };
    processChunk(r, processor);
  };

  auto fudgeChunk = [this](Rect* r, BiomeType* b)
  {
    auto fudgeProcessor = [this](int h, int i, int j)
    {
      auto it = terrainMap[h].find({ i, j });
      if (it == terrainMap[h].end())
      {
        //make tile based on most common biome in range of 1
      }
      else if (it->second.initialized == false)
      {
        Rect range = { i-2, j-2, i+2, j+2 };
        auto t = generateRangeReport(&range, h);
        auto [bCount, topBiomeName] = t.topBiome[h];
        if (it->second.biomeType->name != topBiomeName && cfg->biomeExistsOnLevel(topBiomeName, h))
        {
            if (std::rand() % 10 > 4)
            {
              updateTile(h, i, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
            }
            else
            {
              updateTile(h, i+1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
              updateTile(h, i-1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
              updateTile(h, i, j+1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
              updateTile(h, i, j-1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
            }
        }
      }
    };
    if (std::rand() % 100 > 65) processChunk(r, fudgeProcessor);
  };

  map::chunk::multiprocessFunctorVec terrainPlacement { { createTerrainObjects, [this](map::chunk::ChunkProcessor* p, int z, std::tuple<int, int> coords)
  {
    if (cfg->biomeExistsOnLevel(p->getBrush()->name, z) == false)
      p->setBrush(cfg->getRandomBiomeType(z)); 
    if (std::rand() % 10 > 5)
    {
      auto [i, j] = coords;
      Rect range = { i-5, j-5, i+5, j+5 };
      auto t = generateRangeReport(&range, 0);
      auto [bCount, topBiomeName] = t.topBiome[z];
      if (topBiomeName.length() > 0 && cfg->biomeExistsOnLevel(topBiomeName, z) == true)
        p->setBrush(&cfg->biomeTypes[topBiomeName]);
      else
        p->setBrush(cfg->getRandomBiomeType(z));
    }
    return p->getBrush(); } }

  };
  map::chunk::multiprocessFunctorVec chunkFudging {
    { fudgeChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { hammerChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { fudgeChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { cleanChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} }
  };
  map::chunk::multiprocessFunctorVec objectPlacement { { addMobs, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(); } } };

  map::chunk::ChunkProcessor chunker ( chunkRect, maxDepth );
  chunker.setBrush(cfg->getRandomBiomeType(0));
  SDL_Log("Adding terrain objects...");
  // std::thread t([this, &chunker](multiprocessChain o, multiprocessChain c){ chunker.multiProcessChunk({ o, c }); }, objectPlacers, chunkFuzzers);
  // t.join();
  chunker.multiProcessChunk({ terrainPlacement, chunkFudging });
  SDL_Log("Adding world and mob objects...");
  
  // TODO: Chunkfuzz is fine but not in this case because this is what initializes all tiles. Need something else for that
  chunker.multiProcessChunk({ objectPlacement });
  SDL_Log("Done adding objects.");

  mapGenerator.reset(&mtx);
  std::unique_lock lock(mtx);
  SDL_Log("Created chunk. Map now has %lu terrain objects, %lu world objects, and %lu mob objects for a total of %lu",
    terrainMap[0].size()*2,
    worldMap[0].size()*2,
    terrainMap[0].size()*2,
    terrainMap[0].size()*2+worldMap[0].size()*2+mobMap[0].size()*2
  );
  return 0;
}

////////////////////
//  UI
///////////////////

struct Element
{
  int offsetX;
  int offsetY;
};

struct Button : Element
{
  std::function<void()> onClick;
};

struct TextBox : Element
{
  std::string content;
  int w;
  int h;
  TTF_Font* font;
  void getLines (std::vector<std::string>*);
  TextBox (TTF_Font* f, std::string c, int x, int y, int w, int h)
  {
    font = f;
    offsetX = x;
    offsetY = y;
    content = c;
    // if (!w && !h)
    // {
    //   TTF_SizeUTF8(font, c.c_str(), &w, &h);
    // }
    this->w = w;
    this->h = h;
  }
};

struct UIRect : SDL_Rect
{
  int borderWidth;
  SDL_Color foregroundColor;
  SDL_Color borderColor;
  SDL_Color backgroundColor;
  SDL_Color textColor;
  std::string title;
  std::string content;
  std::vector<Button> buttons;
  std::vector<TextBox> textBoxes;
  TTF_Font* font;
  void getLines (std::vector<std::string>*, TTF_Font*);
  UIRect* setDimensions (int, int, int, int);
  UIRect* setTitle (std::string t) { this->title = t; return this; }
  UIRect* setFont (TTF_Font* f) { this->font = f; return this; }
  UIRect* addTextBox (std::string s, int x = 0, int y = 0, int w = 0, int h = 0) {
    TextBox t{font, s, x, y, w, h};
    textBoxes.push_back(t);
    return this;
  }
  UIRect* addButton (Button b) { buttons.push_back(b); return this; }
};

////////////////////
//  ENGINE
////////////////////

namespace engine
{
  namespace graphics
  {
    template <typename T1> T1 controller;
    struct RenderController;
    struct SurfaceController;
    struct WindowController;
  }
  namespace ui
  {
    template <typename T1> T1 controller;
    struct UIWindowController;
  }
  template <typename T1> T1 controller;
}

struct GameEngine
{
  template <typename T> void registerController (T t)
  {
    engine::controller<T> = t;
  };
  config::ConfigurationController configController;
  input::UserInputHandler userInputHandler;
  bool running;
  int movementSpeed;
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  SDL_Surface* gameSurface;
  SDL_Texture* gameTexture;
  TTF_Font* gameFont;
  Image tilemapImage;
  SDL_Event appEvent;
  SDL_DisplayMode displayMode;
  Player player;
  int tileSize;
  const int spriteSize;
  int zLevel;
  const int zMaxLevel;
  map::MapController mapController;
  objects::mobTypesMap* mobTypes;
  objects::objectTypesMap* objectTypes;
  objects::biomeTypesMap* biomeTypes;
  std::vector<std::string>* biomeTypeKeys;
  objects::terrainTypesMap* terrainTypes;
  objects::tileTypesMap* tileTypes;
  objects::biomeMap* biomeMap;
  objects::terrainMap* terrainMap;
  objects::worldMap* worldMap;
  objects::mobMap* mobMap;
  std::map<int, std::vector<std::shared_ptr<MobObject>>> mobs;
  std::map<std::string, Sprite>* sprites;
  SDL_Rect camera;
  int init();
  std::map<int, std::map<std::string, int>> getTilesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, int>> getBiomesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (SDL_Rect*);
  int generateMapChunk(SDL_Rect*);
  int run();
  bool stopRunning() { running = false; return !running; }
  int getSpriteSize() { return spriteSize; }
  int getTileSize() { return tileSize; }
  template <typename T> T* controller() { return &engine::controller<T>; }
  GameEngine() : running(true), movementSpeed(8), tileSize(32), spriteSize(32), zLevel(0), zMaxLevel(2) {}
};

struct engine::graphics::RenderController
{
  GameEngine* e;
  RenderController () {}
  RenderController (GameEngine* e)
  {
    this->e = e;
  }
  int renderCopySprite(Sprite*, std::tuple<int, int, int, int>);
  int renderCopySprite(Sprite*, int, int);
  int renderCopySprite(std::string, int, int);
  int renderCopyObject(std::shared_ptr<WorldObject>, int, int);
  int renderCopyMobObject(std::shared_ptr<MobObject>, int, int);
  int renderCopyTerrain(TerrainObject*, int, int);
  int renderFillUIWindow(UIRect*);
};

struct engine::graphics::SurfaceController
{
  GameEngine* e;
  SurfaceController () {}
  SurfaceController (GameEngine* e)
  {
    this->e = e;
  }
  SDL_Surface* getGameSurfaceFromWindow ();
  SDL_Texture* getTextureFromSurface (SDL_Surface*);
  SDL_Texture* getGameSurfaceTexture ();
};

struct engine::graphics::WindowController
{
  GameEngine* e;
  WindowController () {}
  WindowController (GameEngine* e)
  {
    this->e = e;
  }
  std::tuple<int, int> getWindowGridDimensions();
  std::tuple<int, int> getWindowDimensions();
};

////////////////////
//  CONTROLLERS
////////////////////

namespace controller
{
  struct CameraController;
  struct EventsController;
  struct MovementController;
  struct RenderController;
  struct GraphicsController;
  struct UIController;
}

struct controller::MovementController
{
  GameEngine* e;
  MovementController () {}
  MovementController (GameEngine* e)
  {
    this->e = e;
  }
  void processMap(int);
  void scrollGameSurface (int);
};

struct controller::UIController
{
  GameEngine* e;
  std::vector<UIRect> windows;
  UIController () {}
  UIController (GameEngine* e)
  {
    this->e = e;
  }
  void createUIWindow (int, int, int, int, std::string = "", std::string = "");
  UIRect* createUIWindow (int = 0, int = 0, int = 0, int = 0);
};

struct controller::GraphicsController
{
  GameEngine *e;
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  SDL_Texture* tilemapTexture;
  Image* tilemapImage;
  SDL_DisplayMode displayMode;
  SDL_Rect camera;
  TTF_Font* font;
  std::map<std::string, Sprite> sprites;
  int windowWidth;
  int windowHeight;
  template <typename T> void registerController (T t)
  {
    engine::graphics::controller<T> = t;
  };
  template <typename T> T* controller() { return &engine::graphics::controller<T>; }
  GraphicsController () {}
  GraphicsController (GameEngine* e);
  void applyUI();
};


struct controller::CameraController
{
  GameEngine* e;
  CameraController () {}
  CameraController (GameEngine* e)
  {
    this->e = e;
  }
  void scrollCamera(int);
  void iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)>);
};

struct controller::RenderController
{
  GameEngine* e;
  RenderController () {}
  RenderController (GameEngine* e)
  {
    this->e = e;
  }
  void renderCopyTiles();
  int renderCopyPlayer();
  int renderUI();
};

void controller::RenderController::renderCopyTiles()
{
  auto processor = [this](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto it = e->mapController.mobMap[e->zLevel][{i, j}].begin();
    while (it != e->mapController.mobMap[e->zLevel][{i, j}].end())
    {
      auto mob = it->get();
      for (auto s : mob->simulators)
        s->simulate();
      auto orders = it->get()->orders;
      if (orders & simulated::MOVE)
      {
        mob->orders -= simulated::MOVE;
        if (!e->mapController.isPassable(std::make_tuple(mob->z,mob->x,mob->y)))
          continue;
        auto direction = i != mob->x
          ? (i < mob->x ? tileObject::RIGHT : tileObject::LEFT)
          : j != mob->y
            ? (j < mob->y ? tileObject::DOWN : tileObject::UP)
            : tileObject::DOWN;

        it = e->mapController.moveMob(it, {e->zLevel, i, j}, direction);
        continue;
      }
      ++it;
    }
  };
  std::thread p (
    [this](std::function<void(std::tuple<int, int, int, int>)> f)
    {
      e->controller<controller::CameraController>()->iterateOverTilesInView(f);
    }, processor
  );
  std::vector<std::pair<std::shared_ptr<MobObject>, std::tuple<int, int>>> movers;
  auto terrainRenderer = [this,&movers](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto terrainObject = e->mapController.terrainMap[e->zLevel].find({ i, j });
    if (terrainObject != e->mapController.terrainMap[e->zLevel].end())
      engine::graphics::controller<engine::graphics::RenderController>.renderCopyTerrain(&terrainObject->second, x, y);
    else {}
      //engine::graphics::controller<engine::graphics::RenderController>.renderCopySprite("Sprite 0x128", x, y);
    auto worldObject = e->mapController.worldMap[e->zLevel].find({ i, j });
    if (worldObject != e->mapController.worldMap[e->zLevel].end())
      for ( auto w : worldObject->second )
        engine::graphics::controller<engine::graphics::RenderController>.renderCopyObject(w, x, y);
    auto mobObject = e->mapController.mobMap[e->zLevel].find({ i, j });
    if (mobObject != e->mapController.mobMap[e->zLevel].end())
      for ( auto &w : mobObject->second )
      {
        if (w->relativeX == 0 && w->relativeY == 0)
          engine::graphics::controller<engine::graphics::RenderController>.renderCopyMobObject(w, x, y);
        else
          movers.push_back({w, { x, y }});
      }
  };
  std::thread r (
    [&movers](std::function<void(std::tuple<int, int, int, int>)> f1)
    {
      engine::controller<controller::CameraController>.iterateOverTilesInView(f1);
      auto it = movers.begin();
      while (it != movers.end())
      {
        auto mob = it->first;
        auto [_x, _y] = it->second;
        engine::graphics::controller<engine::graphics::RenderController>.renderCopyMobObject(mob, _x, _y);
        movers.erase(it);
      }
    },
    terrainRenderer
  );
  p.join();
  r.join();
  SDL_SetRenderDrawColor(e->appRenderer, 0, 0, 0, 255);
}

int controller::RenderController::renderCopyPlayer()
{
  e->player.x = engine::controller<controller::GraphicsController>.camera.x;
  e->player.y = engine::controller<controller::GraphicsController>.camera.y;
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
  SDL_Rect playerRect = { _w/2*e->tileSize, _h/2*e->tileSize, e->tileSize, e->tileSize };
  return SDL_RenderFillRect(e->appRenderer, &playerRect);
}

int controller::RenderController::renderUI()
{

  if (engine::controller<controller::UIController>.windows.size() != 0)
    for (auto w : engine::controller<controller::UIController>.windows)
    {
      engine::graphics::controller<engine::graphics::RenderController>.renderFillUIWindow(&w);
    }
  return 0;
}

controller::GraphicsController::GraphicsController (GameEngine* e)
{
  this->e = e;
  auto renderController = engine::graphics::RenderController(e);
  auto surfaceControlller = engine::graphics::SurfaceController(e);
  auto windowController = engine::graphics::WindowController(e);
  engine::graphics::controller<engine::graphics::RenderController> = renderController;
  engine::graphics::controller<engine::graphics::SurfaceController> = surfaceControlller;
  engine::graphics::controller<engine::graphics::WindowController> = windowController;

  SDL_Log("Initializing SDL libraries...");
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Could not initialize SDL: %s",
      SDL_GetError()
    );
  }
  else
  {
    SDL_Log("SDL initialized.");
  }
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Could not initialize SDL_image: %s",
      IMG_GetError()
    );
  }
  else
  {
    SDL_Log("SDL_image initialized.");
  }
  SDL_GetCurrentDisplayMode(0, &displayMode);
  SDL_Log("Current display is %dx%dpx.",
    displayMode.w,
    displayMode.h
  );
  if(TTF_Init()==-1) {
    SDL_Log("TTF_Init: %s\n", TTF_GetError());
  }
  else
  {
    SDL_Log("SDL_ttf initialized.");
  }
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
  SDL_Log("Current window grid is %dx%d tiles.", _w, _h);
  camera = { 0, 0, _w/e->getTileSize(), _h/e->getTileSize() };
  SDL_Log("Camera created at (%d, %d) with %dx%d tile dimensions.", 0, 0,
    displayMode.w/e->getTileSize(),
    displayMode.h/e->getTileSize()
  );
}

void controller::GraphicsController::applyUI ()
{
  int tS = e->getTileSize();
  SDL_Rect viewportRect;
  SDL_RenderGetViewport(appRenderer, &viewportRect);
  SDL_Rect leftRect = {0, 0, tS, viewportRect.h};
  SDL_Rect rightRect = {viewportRect.w-tS, 0, tS, viewportRect.h};
  SDL_Rect topRect = {0, 0, viewportRect.w, tS};
  SDL_Rect bottomRect = {0, viewportRect.h-tS*2, viewportRect.w, tS*2};
  SDL_RenderFillRect(appRenderer, &leftRect);
  SDL_RenderFillRect(appRenderer, &rightRect);
  SDL_RenderFillRect(appRenderer, &topRect);
  SDL_RenderFillRect(appRenderer, &bottomRect);  
}

void controller::CameraController::scrollCamera(int directions)
{
  int x = engine::controller<controller::GraphicsController>.camera.x;
  int y = engine::controller<controller::GraphicsController>.camera.y;
  if (directions & input::LEFT)
    x--;
  if (directions & input::RIGHT)
    x++;
  if (directions & input::DOWN)
    y++;
  if (directions & input::UP)
    y--;
  if (e->mapController.isPassable({e->zLevel, x, y}))
  {
    if (e->tileSize > 8)
      engine::controller<controller::MovementController>.scrollGameSurface(directions);
    else
      SDL_Delay(15);
    engine::controller<controller::GraphicsController>.camera.x = x;
    engine::controller<controller::GraphicsController>.camera.y = y;
  }
  else
  {
    e->controller<controller::RenderController>()->renderCopyTiles();
    e->controller<controller::RenderController>()->renderCopyPlayer();
    e->controller<controller::GraphicsController>()->applyUI();
    e->controller<controller::RenderController>()->renderUI();
    SDL_RenderPresent(e->appRenderer);    
  }
}

void controller::CameraController::iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)> f)
{
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
  int x = 0;
  int y = 0;
  for (auto i = engine::controller<controller::GraphicsController>.camera.x - _w/2; i < engine::controller<controller::GraphicsController>.camera.x + _w/2 + 5; i++)
  {
    for (auto j = engine::controller<controller::GraphicsController>.camera.y - _h/2; j < engine::controller<controller::GraphicsController>.camera.y + _h/2 + 5; j++) {
      std::tuple<int, int, int, int> locationData = {x, y, i, j};
      f(locationData);
      y++;
    }
    y = 0;
    x++;
  }
}

void controller::MovementController::processMap(int directions)
{
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
  SDL_Point checkCoordinates = { engine::controller<controller::GraphicsController>.camera.x, engine::controller<controller::GraphicsController>.camera.y };
  Rect chunkRect = {
    engine::controller<controller::GraphicsController>.camera.x-e->configController.gameSize*2,
    engine::controller<controller::GraphicsController>.camera.y-e->configController.gameSize*2,
    engine::controller<controller::GraphicsController>.camera.x+e->configController.gameSize*2,
    engine::controller<controller::GraphicsController>.camera.y+e->configController.gameSize*2
  };
  if (directions & input::RIGHT)
    checkCoordinates.x += _w;
    chunkRect.x1 += _w/2;
  if (directions & input::LEFT)
    checkCoordinates.x -= _w;
    chunkRect.x1 -= _w/2;
  if (directions & input::UP)
    checkCoordinates.y -= _h;
    chunkRect.y1 += _h/2;
  if (directions & input::DOWN)
    checkCoordinates.y += _h;
    chunkRect.y1 -= _h/2;
  auto it = e->mapController.terrainMap[e->zLevel].find({ checkCoordinates.x, checkCoordinates.y });
  if (it == e->mapController.terrainMap[e->zLevel].end())
  {
    SDL_Log("Detected ungenerated map: %d %d", checkCoordinates.x, checkCoordinates.y);
    e->mapController.generateMapChunk(&chunkRect);
  }

};

void controller::MovementController::scrollGameSurface(int directions)
{
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowDimensions();
  SDL_Rect dest {0, 0, _w, _h};
  std::pair<int, int> offset = {0, 0};
  if (directions & input::RIGHT)
    offset.first -= e->tileSize;
  if (directions & input::LEFT)
    offset.first += e->tileSize;
  if (directions & input::UP)
    offset.second += e->tileSize;
  if (directions & input::DOWN)
    offset.second -= e->tileSize;
  while (dest.x != offset.first || dest.y != offset.second)
  {
    e->controller<controller::RenderController>()->renderCopyTiles();
    if (directions & input::RIGHT)
      dest.x -= e->movementSpeed;
    if (directions & input::LEFT)
      dest.x += e->movementSpeed;
    if (directions & input::UP)
      dest.y += e->movementSpeed;
    if (directions & input::DOWN)
      dest.y -= e->movementSpeed;
    SDL_RenderCopy(e->appRenderer, engine::graphics::controller<engine::graphics::SurfaceController>.getGameSurfaceTexture(), NULL, &dest);
    e->controller<controller::RenderController>()->renderCopyPlayer();
    engine::controller<controller::GraphicsController>.applyUI();;
    engine::controller<controller::RenderController>.renderUI();
    SDL_RenderPresent(e->appRenderer);
  }
  if (SDL_SetRenderTarget(e->appRenderer, NULL) < 0)
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not reset render target: %s", SDL_GetError());
}

struct controller::EventsController
{
  GameEngine* e;
  EventsController () {}
  EventsController (GameEngine* e)
  {
    this->e = e;
  }
  void handleEvents();
};

void controller::EventsController::handleEvents()
{
  if (e->userInputHandler.lock.elapsed() > 50)
  {
    auto keyboardMovementHandler = [this](int directions)
    {
      if (directions == -1)
      {
        e->stopRunning();
      }
      std::thread graphicalThread([](int d) { engine::controller<controller::CameraController>.scrollCamera(d); }, directions);
      std::thread mapProcessingThread([](int d) { engine::controller<controller::MovementController>.processMap(d); }, directions);
      mapProcessingThread.detach();
      graphicalThread.join();
    };
    e->userInputHandler.handleKeyboardMovement(keyboardMovementHandler);
    auto eventHandler = [this](SDL_Event* event)
    {
      if (event->type == SDL_QUIT)
      {
        SDL_Delay(1500);
        e->stopRunning();
      }
      else if (event->type == SDL_KEYDOWN)
      {
        auto t = &e->mapController.terrainMap[e->zLevel][{engine::controller<controller::GraphicsController>.camera.x, engine::controller<controller::GraphicsController>.camera.y}];
        std::string objs;
        switch(event->key.keysym.sym)
        {
          case SDLK_ESCAPE:
            if (e->controller<controller::UIController>()->windows.size())
            {
              e->controller<controller::UIController>()->windows.pop_back();
              SDL_Delay(10);
            }
            // else
            // {
            //   e->stopRunning();
            // }
            break;
          case SDLK_SPACE:
            SDL_Log(
              "\nCamera: %dx%dx%dx%d\nCurrent terrain type: %s\nCurrent biome type: %s\nCurrent terrain type sprite name: %s\nInitialized: %d",
              engine::controller<controller::GraphicsController>.camera.x,
              engine::controller<controller::GraphicsController>.camera.y,
              engine::controller<controller::GraphicsController>.camera.w,
              engine::controller<controller::GraphicsController>.camera.h,
              t->terrainType->name.c_str(),
              t->biomeType->name.c_str(),
              t->terrainType->getFrame(0)->name.c_str(),
              t->initialized
            );
            break;
          case SDLK_w:
            if (e->zLevel > 0)
            {
              e->zLevel--;
              SDL_Log("You are at level %d", e->zLevel);
            }
            break;
          case SDLK_q:
            if (std::abs(e->zLevel) < static_cast <int>(e->mapController.terrainMap.size()))
            {
              if (e->zLevel < e->zMaxLevel - 1)
              {
                e->zLevel++;
              }
              SDL_Log("You are at level %d", e->zLevel);
            }
            break;
          case SDLK_p:
            e->tileSize = e->tileSize / 2;
            e->configController.tileSize = e->tileSize;
            break;
          case SDLK_o:
            e->tileSize = e->tileSize * 2;
            e->configController.tileSize = e->tileSize;
            break;
        }
      }
    };
    e->userInputHandler.handleAppEvents(eventHandler);
    e->userInputHandler.lock.reset();
  }
}

void TextBox::getLines (std::vector<std::string>* l)
{
  auto process = [this, l](int r = 0)
  {
    std::string offsetString = content.substr(r) + " ";
    SDL_Log("Processing string: '%s'", offsetString.c_str());
    int lineWidth, lineHeight;
    TTF_SizeUTF8(font, offsetString.c_str(), &lineWidth, &lineHeight);
    int totalLines = 1 + std::floor(lineWidth / this->w);
    int lineLen = offsetString.length() / totalLines;
    int i = 0;
    int offset = 0;
    SDL_Log("Calculations:\nString width: %d\nTextBox width: %d\nLines necessary: %d\nMax length of each line: %d",  lineWidth, this->w, totalLines, lineLen);
    while (i < totalLines)
    {
      int a = 0;
      int b = offset;
      int lineIndex = (i*lineLen)+b;
      if (lineIndex < 0)
      {
        lineIndex = 0;
      }
      std::string line;
      try
      {
        line = offsetString.substr(lineIndex, lineLen);
      }
      catch (const std::out_of_range& err)
      {
        line = offsetString;
      }
      if (line.find(" ") == std::string::npos)
      {
        line += " ";
      }
      // Calculate word-break
      while (line.length() && line.substr(line.length()-1) != " ")
      {
        a--;
        offset--;
        int reducedLineLength = lineLen + a;
        if (lineIndex + reducedLineLength > offsetString.length())
        {
          reducedLineLength = offsetString.length() - lineIndex;
        }
        r = lineIndex + reducedLineLength;
        line = offsetString.substr(lineIndex, reducedLineLength);
      }
      SDL_Log("Created line: '%s'", line.c_str());
      l->push_back(line);
      ++i;
    }
    SDL_Log("Remainder index: %d", r);
    return r;
  };
  int remainderIndex = process();
  SDL_Log("remainderIndex: %d", remainderIndex);
  if (remainderIndex > 0)
    while (remainderIndex != process(remainderIndex))
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Processing line. remainderIndex: %d", remainderIndex);
}

void UIRect::getLines (std::vector<std::string>* l, TTF_Font* f)
{
  auto process = [this, l, f](int r = 0)
  {
    std::string offsetString = content.substr(r) + " ";
    int lineWidth, lineHeight;
    TTF_SizeUTF8(f, offsetString.c_str(), &lineWidth, &lineHeight);
    int totalLines = 1 + lineWidth / w;
    int lineLen = offsetString.length() / totalLines;
    int i = 0;
    int offset = 0;
    while (i < totalLines)
    {
      int a = 0;
      int b = offset;
      int lineIndex = (i*lineLen)+b;
      if (lineIndex < 0) lineIndex = 0;
      std::string line = offsetString.substr(lineIndex, lineLen);
      // Calculate word-break
      while (line.substr(line.length()-1) != " ")
      {
        a--;
        offset--;
        int reducedLineLength = lineLen + a;
        if (lineIndex + reducedLineLength > offsetString.length())
        {
          reducedLineLength = offsetString.length() - lineIndex;
        }
        r = lineIndex + reducedLineLength;
        line = offsetString.substr(lineIndex, reducedLineLength);
      }
      l->push_back(line);
      ++i;
    }
    return r;
  };
  int remainderIndex = process();
  while (remainderIndex != process(remainderIndex))
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Processing line");
}

void controller::UIController::createUIWindow (int x, int y, int w, int h, std::string title, std::string content)
{
  SDL_Color fgColor = { 255, 255, 255 };
  SDL_Color borderColor = { 255, 255, 255, 255 };
  SDL_Color bgColor = { 0, 0, 255, 255 };
  UIRect window = { { x, y, w, h }, 5, fgColor, borderColor, bgColor, borderColor, title, content };
  windows.push_back(window);
}

UIRect* controller::UIController::createUIWindow (int x, int y, int w, int h)
{
  SDL_Color fgColor = { 255, 255, 255 };
  SDL_Color borderColor = { 255, 255, 255, 255 };
  SDL_Color bgColor = { 0, 0, 255, 255 };
  UIRect window = { { x, y, w, h }, 5, fgColor, borderColor, bgColor, borderColor };
  windows.push_back(window);
  return &windows.back();
}

UIRect* UIRect::setDimensions(int x, int y, int w, int h)
{
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
  return this;
}

////////////////////
//  GRAPHICS
////////////////////

int engine::graphics::RenderController::renderCopySprite(Sprite *s, std::tuple<int, int, int, int> coords)
{
  auto [x, y, o_x, o_y] = coords;
  int tS = e->getTileSize();
  SDL_Rect src {s->tileMapX, s->tileMapY, e->getSpriteSize(), e->getSpriteSize()};
  SDL_Rect dest {(x*tS)+o_x, (y*tS)+o_y, tS, tS};
  if (SDL_RenderCopy(e->appRenderer, engine::controller<controller::GraphicsController>.tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int engine::graphics::RenderController::renderCopySprite(Sprite *s, int x, int y)
{
  int tS = e->getTileSize();
  SDL_Rect src {s->tileMapX, s->tileMapY, e->getSpriteSize(), e->getSpriteSize()};
  SDL_Rect dest {x*tS, y*tS, tS, tS};
  if (SDL_RenderCopy(e->appRenderer, engine::controller<controller::GraphicsController>.tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int engine::graphics::RenderController::renderCopySprite(std::string spriteName, int x, int y)
{
  auto it = e->sprites->find(spriteName);
  if (it != e->sprites->end())
    return renderCopySprite(&it->second, x, y);
  else
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not render sprite: %s", spriteName.c_str());
    return -1;
}

int engine::graphics::RenderController::renderCopyObject(std::shared_ptr<WorldObject> t, int x, int y)
{
  if (!t->isAnimated())
    return renderCopySprite(t->objectType->getFrame(0), x, y);
  else
  {
    if (t->animationTimer.elapsed() > t->animationSpeed)
    {
      t->animationTimer.stop();
      t->animationTimer.start();
      t->animationFrame++;
      if (t->animationFrame >= t->objectType->maxFrames())
        t->animationFrame = 0;
    }
    auto it = t->objectType->animationMap[t->direction].find(t->animationFrame);
    if (it == t->objectType->animationMap[t->direction].end())
      return renderCopySprite(t->objectType->getFrame(0), x, y);
    else
      return renderCopySprite(it->second, x, y);
  }
}

int engine::graphics::RenderController::renderCopyMobObject(std::shared_ptr<MobObject> t, int x, int y)
{
  int o_x, o_y = 0;
  if (t->relativeX != 0 || t->relativeY != 0)
  {
    o_x = t->relativeX;
    o_y = t->relativeY;
    if (t->relativeX > 0) t->relativeX -= std::floor(e->getTileSize()/8);
    if (t->relativeX < 0) t->relativeX += std::floor(e->getTileSize()/8);
    if (t->relativeY > 0) t->relativeY -= std::floor(e->getTileSize()/8);
    if (t->relativeY < 0) t->relativeY += std::floor(e->getTileSize()/8);
  }
  if (!t->isAnimated())
    return renderCopySprite(t->mobType->getFrame(0), x, y);
  else
  {
    if (t->animationTimer.elapsed() > t->animationSpeed)
    {
      t->animationTimer.stop();
      t->animationTimer.start();
      t->animationFrame++;
      if (t->animationFrame >= t->mobType->maxFrames(t->direction))
        t->animationFrame = 0;
    }
    auto it = t->mobType->animationMap[t->direction].find(t->animationFrame);
    if (it == t->mobType->animationMap[t->direction].end())
      return renderCopySprite(t->mobType->getFrame(0), { x, y, o_x, o_y });
    else
      return renderCopySprite(it->second, { x, y, o_x, o_y });
  }
}

int engine::graphics::RenderController::renderCopyTerrain(TerrainObject* t, int x, int y) {
  if (!t->isAnimated())
    return renderCopySprite(t->terrainType->getFrame(0), x, y);
  else
  {
    if (t->animationTimer.elapsed() > t->animationSpeed)
    {
      t->animationTimer.stop();
      t->animationTimer.start();
      t->animationFrame++;
      if (t->animationFrame >= t->terrainType->maxFrames())
        t->animationFrame = 0;
    }
    auto it = t->terrainType->animationMap[t->direction].find(t->animationFrame);
    if (it == t->terrainType->animationMap[t->direction].end())
      return renderCopySprite(t->terrainType->getFrame(0), x, y);
    else
      return renderCopySprite(it->second, x, y);
  }
}

int engine::graphics::RenderController::renderFillUIWindow(UIRect* window)
{
  SDL_Rect shadow {window->x, window->y, window->w, window->h+5};
  SDL_SetRenderDrawColor(engine::controller<controller::GraphicsController>.appRenderer, 0, 0, 0, 128);
  SDL_RenderFillRect(
    engine::controller<controller::GraphicsController>.appRenderer,
    &shadow
  );
  SDL_Rect border {window->x, window->y, window->w, window->h};
  SDL_SetRenderDrawColor(
    engine::controller<controller::GraphicsController>.appRenderer,
    window->borderColor.r,
    window->borderColor.g,
    window->borderColor.b,
    window->borderColor.a
  );
  SDL_RenderFillRect(
    engine::controller<controller::GraphicsController>.appRenderer,
    &border
  );
  SDL_Rect bezel {window->x+2, window->y+2,window->w-4, window->h-4};
  SDL_SetRenderDrawColor(
    engine::controller<controller::GraphicsController>.appRenderer,
    window->backgroundColor.r,
    window->backgroundColor.g,
    window->backgroundColor.b,
    std::floor(window->backgroundColor.a / 2)
  );
  SDL_RenderFillRect(
    engine::controller<controller::GraphicsController>.appRenderer,
    &bezel
  );
  SDL_Rect w {window->x+4, window->y+4,window->w-8, window->h-8};
  SDL_SetRenderDrawColor(
    engine::controller<controller::GraphicsController>.appRenderer,
    window->backgroundColor.r,
    window->backgroundColor.g,
    window->backgroundColor.b,
    window->backgroundColor.a
  );
  SDL_RenderFillRect(
    engine::controller<controller::GraphicsController>.appRenderer,
    &w
  );
  SDL_Surface* title = TTF_RenderText_Solid(e->gameFont, window->title.c_str(), window->foregroundColor);
  SDL_Texture* tex = SDL_CreateTextureFromSurface(e->appRenderer, title);
  if (!title)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid error: %s", TTF_GetError());
  }
  else
  {
    SDL_Rect titleRect {window->x+5, window->y+5, title->w, title->h};
    SDL_RenderCopy(e->appRenderer, tex, NULL, &titleRect);
  }

  for (auto box : window->textBoxes)
  {
    std::vector<std::string> lines;
    box.getLines(&lines);
    int _, lineHeight;
    TTF_SizeUTF8(e->gameFont, "test", &_, &lineHeight);
    int i = 0;
    for ( auto l : lines )
    {
      SDL_Surface* textSurface = TTF_RenderText_Solid(e->gameFont, l.c_str(), window->foregroundColor);
      SDL_Texture* textTexture = SDL_CreateTextureFromSurface(e->appRenderer, textSurface);
      SDL_Rect lineRect {
        window->x + box.offsetX + 15,
        window->y + box.offsetY + 15 + lineHeight * i + 5,
        textSurface->w,
        textSurface->h
      };
      SDL_RenderCopy(e->appRenderer, textTexture, NULL, &lineRect);
      SDL_FreeSurface(textSurface);
      SDL_DestroyTexture(textTexture);
      ++i;
    }
  }

  SDL_FreeSurface(title);
  SDL_DestroyTexture(tex);
  return 0;
}

SDL_Surface* engine::graphics::SurfaceController::getGameSurfaceFromWindow ()
{
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
      auto rmask = 0xff000000;
      auto gmask = 0x00ff0000;
      auto bmask = 0x0000ff00;
      auto amask = 0x000000ff;
  #else
      auto rmask = 0x000000ff;
      auto gmask = 0x0000ff00;
      auto bmask = 0x00ff0000;
      auto amask = 0xff000000;
  #endif
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowDimensions();
  //auto [_w, _h] = e->controller<controller::GraphicsController>()->controller<engine::graphics::WindowController>()->getWindowDimensions();
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Current window is %dx%dpx.", _w, _h );
  SDL_Surface* s = SDL_CreateRGBSurface(0, _w, _h, 32, rmask, gmask, bmask, amask);
  if (s == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create RGB surface: %s", SDL_GetError());
    return NULL;
  }
  return s;
}

SDL_Texture* engine::graphics::SurfaceController::getTextureFromSurface (SDL_Surface* s)
{
  if (SDL_MUSTLOCK(s))
  {
    if (SDL_LockSurface(s) < 0)
      return NULL;
    if (SDL_RenderReadPixels(e->appRenderer, NULL, SDL_PIXELFORMAT_RGBA32, s->pixels, s->pitch) < 0)
      return NULL;
    SDL_UnlockSurface(s);
  }
  else
  {
    if (SDL_RenderReadPixels(e->appRenderer, NULL, SDL_PIXELFORMAT_RGBA32, s->pixels, s->pitch) < 0)
      return NULL;
  }
  SDL_Texture* t = SDL_CreateTextureFromSurface(e->appRenderer, s);
  return t;
}

SDL_Texture* engine::graphics::SurfaceController::getGameSurfaceTexture ()
{
  SDL_Surface* s = getGameSurfaceFromWindow();
  if (s)
    return getTextureFromSurface(s);
  else
    return NULL;
}

std::tuple<int, int> engine::graphics::WindowController::getWindowGridDimensions()
{
  auto gfx = e->controller<controller::GraphicsController>();
  SDL_GetWindowSize(e->appWindow, &gfx->windowWidth, &gfx->windowHeight);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Got window size: %dx%d", gfx->windowWidth, gfx->windowHeight
  );
  int _w = static_cast <int> (std::floor(gfx->windowWidth/e->getTileSize()));
  int _h = static_cast <int> (std::floor(gfx->windowHeight/e->getTileSize()));
  return std::make_tuple(_w, _h);
}

std::tuple<int, int> engine::graphics::WindowController::getWindowDimensions()
{
  SDL_Rect viewportRect;
  SDL_RenderGetViewport(e->appRenderer, &viewportRect);
  int _w = viewportRect.w;
  int _h = viewportRect.h;
  return std::make_tuple(_w, _h);
}

int GameEngine::init()
{
  auto cameraController = controller::CameraController(this);
  auto eventsController = controller::EventsController(this);
  auto movementController = controller::MovementController(this);
  auto renderController = controller::RenderController(this);
  auto graphicsController = controller::GraphicsController(this);
  auto uiController = controller::UIController(this);
  registerController<controller::CameraController>(cameraController);
  registerController<controller::EventsController>(eventsController);
  registerController<controller::MovementController>(movementController);
  registerController<controller::RenderController>(renderController);
  registerController<controller::GraphicsController>(graphicsController);
  registerController<controller::UIController>(uiController);

  std::srand(std::time(nullptr));
  if (!tileSize)
    tileSize = spriteSize;
  if (movementSpeed > tileSize)
    movementSpeed = tileSize;

  // Create app window and renderer
  appWindow = SDL_CreateWindow(
    "tile-project", 0, 0, engine::controller<controller::GraphicsController>.displayMode.w/2, engine::controller<controller::GraphicsController>.displayMode.h/2, SDL_WINDOW_RESIZABLE
  ); // SDL_WINDOW_FULLSCREEN
  if (appWindow == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create app window: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Window created.");
  engine::controller<controller::GraphicsController>.appWindow = appWindow;
  appRenderer = SDL_CreateRenderer(appWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC); // SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
  if (appRenderer == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create a enderer: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Renderer created.");
  SDL_SetRenderDrawBlendMode(appRenderer, SDL_BLENDMODE_BLEND);
  gameTexture = SDL_CreateTexture(appRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, engine::controller<controller::GraphicsController>.windowWidth, engine::controller<controller::GraphicsController>.windowHeight);
  SDL_SetTextureBlendMode(gameTexture, SDL_BLENDMODE_BLEND);
  engine::controller<controller::GraphicsController>.appRenderer = appRenderer;

  // Load spritesheet
  SDL_Surface *surface = IMG_Load("assets/tilemap640x640x32.png");
  if (!surface)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMG_Load error: %s", IMG_GetError());
    return 3;
  }
  else
  {
    SDL_Log("Loaded spritesheet is %dx%dpx sheet of %dx%dpx tiles.",
      surface->w,
      surface->h,
      spriteSize,
      spriteSize
    );
  }

  gameFont = TTF_OpenFont("assets/FifteenNarrow.ttf", 16);
  if (!gameFont)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_OpenFont error: %s", TTF_GetError());
    return 3;    
  }

  // Create sprites from spritesheet
  SDL_Texture *texture = SDL_CreateTextureFromSurface(appRenderer, surface);
  if (!texture)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Tilemap loaded.");
  tilemapImage = { surface, texture };
  engine::controller<controller::GraphicsController>.tilemapImage = &tilemapImage;
  engine::controller<controller::GraphicsController>.tilemapTexture = texture;
  std::map<std::string, Sprite> spriteMap;
  for (auto i = 0; i < surface->w; i += spriteSize)
  {
    for (auto j = 0; j < surface->h; j += spriteSize)
    {
      std::string name = "Sprite " + std::to_string(i) + "x" + std::to_string(j);
      Sprite s { i, j, name };
      spriteMap[name] = s;
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Created sprite: %s", name.c_str());
    }
  }
  engine::controller<controller::GraphicsController>.sprites = spriteMap;
  SDL_Log("Spritesheet processed.");

  SDL_Log("Reading tilemap configuration file and creating tiles from sprites.");
  configController = config::ConfigurationController("tilemap.config.json", spriteMap);
  auto [biomeTypes, biomeTypeKeys, terrainTypes, mobTypes, objectTypes, tileTypes] = configController.getTypeMaps();
  player = {configController.gameSize/2, configController.gameSize/2, &configController.tileTypes["water"]};
  mapController = map::MapController(
    zMaxLevel, mobTypes, objectTypes, biomeTypes, biomeTypeKeys, terrainTypes, tileTypes, &configController
  );

  // Create default tilemap
  SDL_Log("Generating default tilemap...");
  Rect initialChunk = { 0 - configController.gameSize, 0 - configController.gameSize, configController.gameSize, configController.gameSize };
  mapController.generateMapChunk(&initialChunk);
  auto UI = &engine::controller<controller::UIController>;
  // UI->createUIWindow()
  //   ->setDimensions(30, 30, 125, 300)
  //   ->setTitle("Welcome!")
  //   ->setFont(gameFont) //
  //   ->addTextBox("This is a demo of `tile-project`.", 10, 10, 100, 200);
  SDL_Log("Tilemap created.");
  return 0;
}

int GameEngine::run ()
{
  init();
  while (running)
  {
    controller<controller::EventsController>()->handleEvents();
    SDL_RenderClear(appRenderer);
    controller<controller::RenderController>()->renderCopyTiles();
    controller<controller::RenderController>()->renderCopyPlayer();
    engine::controller<controller::GraphicsController>.applyUI();
    engine::controller<controller::RenderController>.renderUI();
    SDL_RenderPresent(appRenderer);
  }
  return 1;
}

////////////////////
//  MAIN
////////////////////

int main(int argc, char *argv[])
{
  GameEngine engine;
  engine.run();
  return 0;
}