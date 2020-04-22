#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "SDL2/SDL.h"
#include "objects.h"
#include <functional>
#include <mutex>
#include <thread>

struct ChunkReport
{
  std::map<int, std::map<std::string, std::map<std::string, int>>> counts;
  std::map<int, std::map<std::string, std::string>> top;
  std::map<int, std::map<std::string, std::string>> bottom;
};


struct MapGenerator
{
  bool processing;
  BiomeType* currentBiomeType;
  std::vector<SDL_Rect*> rects;
  MapGenerator () : processing(false), currentBiomeType(NULL) {}
  void init(BiomeType* b, std::mutex* mtx)
  {
    mtx->lock();
    processing = true;
    currentBiomeType = b;
    mtx->unlock();
  }
  void reset(std::mutex* mtx)
  {
    mtx->lock();
    processing = false;
    //currentBiomeType = NULL; // bad idea, segfault
    mtx->unlock();
  }
  bool currentlyGenerating () { return processing; }
  bool isOutOfDepth (int h) { return (h > currentBiomeType->maxDepth || h < currentBiomeType->minDepth); }
  void setCurrentBiomeType (BiomeType* b, std::mutex* mtx) { mtx->lock(); currentBiomeType = b; mtx->unlock(); }
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
  BiomeType* getRandomBiomeType() { return &biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]]; }
  void updateTile (int, int, int, BiomeType*, TileType*, TerrainType*, std::vector<std::shared_ptr<WorldObject>>);
  std::map<int, std::map<std::string, int>> getTilesInRange (SDL_Rect*);
  ChunkReport getChunkReport (SDL_Rect*);
  std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (SDL_Rect*);
  std::map<int, std::map<std::string, int>> getBiomesInRange (SDL_Rect* rangeRect);
  void processChunk(SDL_Rect*, std::function<void(int, int, int)>);
  template<typename F>
  void iterateOverChunk(SDL_Rect*, F);
  void randomlyAccessAllTilesInChunk(SDL_Rect*, std::function<void(int, int, int)>);
  std::map<int, std::vector<SDL_Point>> getAllPointsInRect(SDL_Rect*);
  int generateMapChunk(SDL_Rect*);
};

#endif
