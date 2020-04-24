#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <memory>

struct Rect
{
  int x1;
  int y1;
  int x2;
  int y2;
  std::vector<Rect> rects;
  Rect () {}
  Rect (int a, int b, int c, int d) { x1 = a; y1 = b; x2 = c; y2 = d; }
  SDL_Rect* getSDL_Rect () { SDL_Rect* r; r->x = x1; r->y = y1; r->w = x2; r->h = y2; return r; }
  int getWidth () { return std::abs(x1) + std::abs(x2); }
  int getHeight () { return std::abs(y1) + std::abs(y2); }
  std::pair<int, int> getDimensions () { return { getWidth(), getHeight() }; }
  std::vector<Rect>* getRects()
  {
    int small_w = 15;
    int small_h = 15;
    int w = getWidth();
    int h = getHeight();
    auto result_w = std::div(w, small_w);
    auto result_h = std::div(h, small_h);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "found %dx%d rects in rect of %dx%d", result_w.quot, result_h.quot, w, h);
    for (auto x = x1; x <= x2; x += result_w.quot)
      for (auto y = y1; y <= y2; y += result_h.quot)
      {
        Rect r { x, y, x + result_w.quot, y + result_h.quot };
        rects.push_back(r);
      }
    return &rects;
  }
};

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
  TerrainType* terrainType;
  bool seen;
  bool initialized;
  Tile() : seen(false), initialized(false) {}
  Tile (int x, int y) { this->x = x; this->y = y; }
};

struct TerrainObject : Tile
{
  BiomeType* biomeType;
  TerrainObject () {}
  TerrainObject (int x, int y, BiomeType* b, TerrainType* t)
  {
    this->x = x;
    this->y = y;
    biomeType = b;
    terrainType = t;
  }
};

struct WorldObject : Tile
{
  ObjectType* objectType;
  BiomeType* biomeType;
  WorldObject() {}
  WorldObject(int x, int y, ObjectType* o, BiomeType* b)
  {
    this->x = x;
    this->y = y;
    objectType = o;
    biomeType = b;
  }
};


struct TileObject : Tile
{
  std::vector<TerrainObject> terrainObjects;
  std::vector<std::shared_ptr<WorldObject>> worldObjects;
  void setTerrainObject (TerrainObject t) { if (!terrainObjects.size()) terrainObjects.push_back(t); else terrainObjects.at(0) = t; }
  BiomeType* getBiomeType () { if (!terrainObjects.size()) return nullptr; else return terrainObjects.at(0).biomeType; }
  TerrainType* getTerrainType () { if (!terrainObjects.size()) return nullptr; else return terrainObjects.at(0).terrainType; }
  Sprite* getTerrainTypeSprite () { if (!terrainObjects.size()) return nullptr; else return terrainObjects.at(0).terrainType->sprite; }
  TerrainObject* getTerrainObject () { if (!terrainObjects.size()) return nullptr; else return &terrainObjects.at(0); }
  void addWorldObject (std::shared_ptr<WorldObject> o) { worldObjects.push_back(o); }
  void pruneWorldObjects ()
  {
    for (auto &&it = worldObjects.begin(); it != worldObjects.end();) {
      if (it->get()->biomeType->name != getBiomeType()->name)
        it = worldObjects.erase(it);
      else
        ++it;
    }
  }
  std::vector<std::shared_ptr<WorldObject>>* getWorldObjects () { if (!worldObjects.size()) return nullptr; else return &worldObjects; }
};

struct Player {
  int x;
  int y;
  TileType* tileType;
};

namespace objects
{
  typedef std::map<std::string, ObjectType> objectTypesMap;
  typedef std::map<std::string, BiomeType> biomeTypesMap;
  typedef std::map<std::string, TerrainType> terrainTypesMap;
  typedef std::map<std::string, TileType> tileTypesMap;
  typedef std::vector<std::shared_ptr<WorldObject>> objectsVector;
  typedef std::map<int, std::map<std::pair<int, int>, TileObject>> tileMap;
}

#endif