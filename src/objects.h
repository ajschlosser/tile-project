#include <map>
#include <string>
#include <vector>

struct Sprite;
struct GenericType
{
  Sprite* sprite;
  std::string name;
};

struct TileType;
struct ObjectType : GenericType
{
  std::vector<TileType*> tiles;
};

struct TileType : GenericType
{
  std::vector<std::string> objects;
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
};

struct TerrainObject : Tile { };

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