#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "SDL2/SDL.h"
#include "objects.h"
#include "config.h"
#include "rect.h"

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <variant>

#include "map/chunk/chunk.h"

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

#endif
