#include <map>
#include <string>
#include <vector>

struct Sprite;
struct GenericType
{
  Sprite* sprite;
  std::string name;
};

struct ObjectType : GenericType
{
  std::map<std::string, int> biomes;
};

struct TileType : GenericType
{
  std::vector<std::string> objects;
};

struct TerrainType : GenericType
{
  std::vector<std::string> objects;
  TerrainType () {}
  TerrainType (Sprite* sprite, std::string name, std::vector<std::string> objects)
  {
    this->sprite = sprite;
    this->name = name;
    this->objects = objects;
  }
};

struct BiomeType
{
  std::string name;
  int maxDepth;
  int minDepth;
  std::map<int, std::pair<std::string, float>> terrainTypes;
};

struct Tile
{
  int x;
  int y;
  TileType* tileType;
  std::shared_ptr<TerrainType> terrainType;
  Tile() {}
  Tile(int x, int y, TileType* tileType)
  {
    this->x = x;
    this->y = y;
    this->tileType = tileType;
  }
  std::shared_ptr<TerrainType>* getTerrainType() { return &terrainType; }
};

struct TerrainObject : Tile
{
  BiomeType* biomeType;
};

struct WorldObject : Tile
{
  ObjectType* objectType;
  WorldObject() {}
  WorldObject(int x, int y, TileType* tileType)
  {
    this->x = x;
    this->y = y;
    this->tileType = tileType;
  }
};

struct Player {
  int x;
  int y;
  TileType* tileType;
};