#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include <map>
#include <string>
#include <vector>
#include <memory>

struct Image
{
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite
{
  int tileMapX;
  int tileMapY;
  std::string tileName;
};

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
  TileType () {}
  TileType (Sprite* s, std::string n)
  {
    sprite = s;
    name = n;
  }
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
  BiomeType () {}
};

struct Tile
{
  int x;
  int y;
  TileType* tileType;
  TerrainType* terrainType;
  Tile() {}
  Tile (int x, int y) { this->x = x; this->y = y; }
  Tile(int x, int y, TileType* t)
  {
    this->x = x;
    this->y = y;
    tileType = t;
  }
};

struct TerrainObject : Tile
{
  BiomeType* biomeType;
  TerrainObject () {}
  TerrainObject (int x, int y, BiomeType* b)
  {
    this->x = x;
    this->y = y;
    biomeType = b;
    
    // set terraintype so no fuckup
    Sprite s = { 0, 0 };
    std::vector<std::string> o;
    TerrainType t = { &s, "fucko", o };
    terrainType = &t;
  }
};

struct WorldObject : Tile
{
  ObjectType* objectType;
  WorldObject() {}
  WorldObject(int x, int y, TileType* t)
  {
    this->x = x;
    this->y = y;
    tileType = t;
  }
};

struct Player {
  int x;
  int y;
  TileType* tileType;
};

#endif