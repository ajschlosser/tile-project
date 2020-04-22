#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "SDL2/SDL.h"
#include "objects.h"
#include <functional>

struct MapGenerator
{
  bool processing;
  BiomeType* currentBiomeType;
  std::vector<SDL_Rect*> rects;
  MapGenerator () : processing(false), currentBiomeType(NULL) {}
  void init(BiomeType* b)
  {
    processing = true;
    currentBiomeType = b;
  }
  void reset()
  {
    processing = false;
    currentBiomeType = NULL;
  }
  bool currentlyGenerating () { return processing; }
  bool isOutOfDepth (int h) { return (h > currentBiomeType->maxDepth || h < currentBiomeType->minDepth); }
};

struct MapController
{
  int maxDepth;
  std::map<std::string, ObjectType> objectTypes;
  std::map<std::string, BiomeType> biomeTypes;
  std::vector<std::string> biomeTypeKeys;
  std::map<std::string, TerrainType> terrainTypes;
  std::map<std::string, TileType> tileTypes;
  std::map<int, std::map<std::pair<int, int>, TerrainObject>> terrainMap;
  std::map<int, std::map<std::pair<int, int>, std::map<int, std::shared_ptr<WorldObject>>>> objectMap;
  MapGenerator mapGenerator;
  MapController () : maxDepth(0) {}
  MapController (
      int d,
      objects::objectTypesMap oT,
      objects::biomeTypesMap bT,
      std::vector<std::string> bTK,
      objects::terrainTypesMap tT,
      objects::tileTypesMap tTT
  )
  {
    maxDepth = d; objectTypes = oT; biomeTypes = bT; biomeTypeKeys = bTK;
    terrainTypes = tT; tileTypes = tTT;
  }
  void updateTile (int, int, int, BiomeType*, TileType*, TerrainType*, std::vector<std::shared_ptr<WorldObject>>);
  std::map<int, std::map<std::string, int>> getTilesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (SDL_Rect*);
  std::map<int, std::map<std::string, int>> getBiomesInRange (SDL_Rect* rangeRect);
  void processChunk(SDL_Rect*, std::function<void(int, int, int)>);
  void iterateOverChunk(SDL_Rect*, std::function<void(int, int, int)>);
  void randomlyAccessAllTilesInChunk(SDL_Rect*, std::function<void(int, int, int)>);
  std::map<int, std::vector<SDL_Point>> getAllPointsInRect(SDL_Rect*);
  int generateMapChunk(SDL_Rect*);
};

#endif
