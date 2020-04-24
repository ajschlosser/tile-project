#ifndef GAME_CHUNK_H
#define GAME_CHUNK_H

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
  int zMax;;
  ChunkProcessor (Rect* r, int zMax = 3)
  {
    chunk = r;
    smallchunks = r->getRects();
    this->zMax = zMax;
  }
  void processEdges(Rect*, std::pair<chunkProcessorFunctor, BiomeType*>);
  void multiProcess (Rect*, std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>>, 2>);
  void process (Rect*, std::vector<chunkFunctor> functors);
  void processChunk (std::vector<chunkFunctor> functors) { process(chunk, functors); }
  void multiProcessChunk (std::array<std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>>, 2> functors) { multiProcess(chunk, functors); }
};

#endif