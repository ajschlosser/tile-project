#ifndef GAME_CHUNK_H
#define GAME_CHUNK_H

#include "SDL2/SDL.h"
#include "objects.h"
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <array>
#include <variant>

typedef std::function<void(int, int, int)> chunkFunctor;
typedef std::function<void(int, int, int, BiomeType*)> chunkProcessorFunctor;
typedef std::function<void(Rect*, BiomeType* b)> chunkProcessorCallbackFunctor;
typedef std::variant<chunkFunctor, chunkProcessorFunctor, chunkProcessorCallbackFunctor> genericChunkFunctor;

namespace chunk {

struct ChunkReport
{
  std::map<int, std::map<std::string, int >> terrainCounts;
  std::map<int, std::map<std::string, int >> biomeCounts;
  std::tuple<int, std::string> topTerrain;
  std::tuple<int, std::string> topBiome;
  std::map<std::string, std::string> meta;
};

struct ChunkProcessor
{
  Rect* chunk;
  std::vector<Rect>* smallchunks;
  int zMax;
  BiomeType* brush;
  std::shared_mutex brushMtx;
  ChunkProcessor (Rect* r, int zMax = 3)
  {
    chunk = r;
    bool shuffle = true;
    smallchunks = r->getRects(shuffle);
    this->zMax = zMax;
  };
  BiomeType* getBrush() { std::shared_lock lock(brushMtx); return brush; }
  void setBrush(BiomeType* b)
  {
    std::unique_lock lock(brushMtx);
    brush = b;
  }
  void processEdges(Rect*, std::pair<chunkProcessorFunctor, BiomeType*>);
  void multiProcess (Rect*, std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*(chunk::ChunkProcessor*)>>>, 2>, int);
  void lazyProcess (Rect* r, std::vector<chunkFunctor>, int);
  void process (Rect*, std::vector<chunkFunctor>);
  void processChunk (std::vector<chunkFunctor> functors) { process(chunk, functors); }
  void lazyProcessChunk (std::vector<chunkFunctor> functors, int fuzz = 1) { lazyProcess(chunk, functors, fuzz); }
  void multiProcessChunk (std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*(chunk::ChunkProcessor*)>>>, 2> functors, int fuzz = 1) { multiProcess(chunk, functors, fuzz); }
};

ChunkReport getRangeReport(std::function<void(int, int, ChunkReport*)>, Rect*, int, int);
}

#endif