#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "SDL2/SDL.h"
#include "objects.h"
#include "chunk.h"

#include <functional>
#include <mutex>
#include <thread>

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
    mtx->unlock();
  }
  bool currentlyGenerating () { return processing; }
  bool isOutOfDepth (int h) { return (h > currentBiomeType->maxDepth || h < currentBiomeType->minDepth); }
  void setCurrentBiomeType (BiomeType* b, std::mutex* mtx) { mtx->lock(); currentBiomeType = b; mtx->unlock(); }
};

struct MapController
{
  int maxDepth;
  objects::objectTypesMap objectTypes;
  std::map<std::string, BiomeType> biomeTypes;
  std::vector<std::string> biomeTypeKeys;
  objects::terrainTypesMap terrainTypes;
  objects::tileTypesMap tileTypes;
  objects::tileMap tileMap;
  MapGenerator mapGenerator;
  MapController () : maxDepth(0) {}
  MapController (
      int d,
      objects::objectTypesMap oTypes,
      objects::biomeTypesMap bTypes,
      std::vector<std::string> bTypeKeys,
      objects::terrainTypesMap tnTypes,
      objects::tileTypesMap tlTypes
  )
  {
    maxDepth = d; objectTypes = oTypes; biomeTypes = bTypes; biomeTypeKeys = bTypeKeys;
    terrainTypes = tnTypes; tileTypes = tlTypes;
  }
  BiomeType* getRandomBiomeType() { return &biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]]; }
  void updateTile (int, int, int, BiomeType*, TerrainType*, std::vector<std::shared_ptr<WorldObject>>);
  std::map<int, std::map<std::string, int>> getTilesInRange (Rect*);
  ChunkReport getChunkReport (Rect*);
  std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (Rect*);
  std::map<int, std::map<std::string, int>> getBiomesInRange (Rect* rangeRect);
  void processChunk(Rect*, std::function<void(int, int, int)>);
  template<typename F> void iterateOverChunk(Rect*, F);
  template<typename F> void iterateOverChunkEdges(Rect*, F);
  void randomlyAccessAllTilesInChunk(Rect*, std::function<void(int, int, int)>);
  std::map<int, std::vector<SDL_Point>> getAllPointsInRect(Rect*);
  int generateMapChunk(Rect*);
};

#endif
