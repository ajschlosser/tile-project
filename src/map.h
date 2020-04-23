#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "SDL2/SDL.h"
#include "objects.h"
#include <functional>
#include <mutex>
#include <thread>
#include <array>
#include <variant>

typedef std::function<void(int, int, int)> chunkFunctor;
typedef std::function<void(int, int, int, BiomeType*)> chunkProcessorFunctor;
typedef std::function<void(Rect*, std::vector<chunkProcessorFunctor>)> chunkProcessorCallbackFunctor;
typedef std::variant<chunkFunctor, chunkProcessorFunctor, chunkProcessorCallbackFunctor> genericChunkFunctor;

struct ChunkReport
{
  std::map<int, std::map<std::string, std::map<std::string, int>>> counts;
  std::map<int, std::map<std::string, std::string>> top;
  std::map<int, std::map<std::string, std::string>> bottom;
};

struct ChunkProcessor
{
  Rect* chunk;
  std::vector<Rect>* smallchunks;
  int zMax;
  // std::vector<chunkFunctor>* tileFunctor;
  // std::vector<chunkFunctor>* smallchunksFunctor;
  ChunkProcessor (Rect* r, int zMax = 3) //, std::vector<chunkFunctor>* bf, std::vector<chunkFunctor>* sf = nullptr)
  {
    chunk = r;
    smallchunks = r->getRects();
    this->zMax = zMax;
    // tileFunctor = bf;
    // smallchunksFunctor = sf;
  }
  void processEdges(Rect* r, std::pair<chunkProcessorFunctor, BiomeType*> f)
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
        {
          for (auto i = x1; i <= x2; i++)
          {
            for (auto j = y1; j <= y2; j++)
            {
              f.first(h, i, j, f.second);
            }
          }
        }
      }, it->x1, it->y1, it->x2, it->y2);
      t.detach();
    }
  }
  void multiProcess (Rect* r, std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>>, 2> functors)
  {
    for (auto it = smallchunks->begin(); it != smallchunks->end(); ++it)
    {
      for (auto f : functors[0])
      {
        BiomeType* b = f.second();
        std::thread t([this, f, b, functors](int x1, int y1, int x2, int y2) {
          for (auto h = 0; h < zMax; h++)
          {
            for (auto i = x1; i != x2; i++)
            {
              for (auto j = y1; j != y2; j++)
              {
                std::get<chunkProcessorFunctor>(f.first)(h, i, j, b);
              }
            }
          }
        }, it->x1, it->y1, it->x2, it->y2);
        t.detach();
      }
      // chunk postprocess
      for (auto f : functors[1])
      {
        //std::get<chunkProcessorCallbackFunctor>(f.first)(&it, f);
      }
    }
  }
  void process (Rect* r, std::vector<chunkFunctor> functors)
  {
    for (auto h = 0; h < zMax; h++)
    {
      for (auto i = r->x1; i != r->x2; i++)
      {
        for (auto j = r->y1; j != r->y2; j++)
        {
          // for (auto f : functors) f(h, i, j, b);
          for (auto f : functors) f(h, i, j);
        }
      }
    }
  }
  void processChunk (std::vector<chunkFunctor> functors)
  {
    process(chunk, functors);
  }
  void multiProcessChunk (std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>>, 2> functors) // std::vector<chunkFunctor> v1, std::vector<chunkFunctor> v2)
  {

    //for (auto f : functors) multiProcess(chunk, f);
    multiProcess(chunk, functors);

    // process(chunk, v1);
    // for (auto f : v1)
    // {
    //   process(chunk, v1);
    // }
    // if (!funcArr[0].empty())
    // {
    //   for (auto f : funcArr[0])
    //   {
    //     process(chunk, f);
    //   }
    // }
    // if (!funcArr[1].empty())
    // {

    // }
  }
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
