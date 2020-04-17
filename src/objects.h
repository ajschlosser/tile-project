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
  std::map<int, std::pair<std::string, float>> terrainTypes;
};

struct Tile
{
  int x;
  int y;
  std::string type;
  TileType* tileType;
  bool exists ()
  {
    return type.length() > 0;
  }
};

struct TerrainObject : Tile { };

struct WorldObject : Tile
{
  ObjectType* objectType;
};

struct Player {
  int x;
  int y;
  std::string type;
  TileType* tileType;
  int hp;
  bool exists ()
  {
    return type.length() > 0;
  }
};

struct Camera
{
  int x;
  int y;
  int w;
  int h;
};