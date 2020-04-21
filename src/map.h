#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "SDL2/SDL.h"
#include "objects.h"
#include <functional>
#include <memory>

struct MapGenerator
{
  bool processing;
  std::shared_ptr<BiomeType> currentBiomeType;
  MapGenerator () : processing(false) {}
  void init(std::shared_ptr<BiomeType> b)
  {
    processing = true;
    currentBiomeType = b;
  }
  void reset()
  {
    processing = false;
    currentBiomeType = nullptr;
  }
  bool currentlyGenerating () { return processing; }
  bool isOutOfDepth (int h) { return (h > currentBiomeType->maxDepth || h < currentBiomeType->minDepth); }
};

struct MapController
{
  int maxDepth;
  MapGenerator mapGenerator;
  MapController () : maxDepth(0) {}
  MapController (int maxDepth)
  {
    this->maxDepth = maxDepth;
  }
  void iterateOverChunk(SDL_Rect*, std::function<void(int, int, int)>);
  void randomlyAccessAllTilesInChunk(SDL_Rect*, std::function<void(int, int, int)>);
  std::map<int, std::vector<SDL_Point>> getAllPointsInRect(SDL_Rect*);
};

#endif