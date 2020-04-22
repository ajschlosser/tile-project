#include "map.h"
#include <mutex>
#include <thread>

std::mutex mtx;

void MapController::updateTile (int z, int x, int y, BiomeType* biomeType, TileType* tileType = NULL, TerrainType* terrainType = NULL, objects::objectsVector worldObjects = objects::objectsVector ())
{
  TerrainObject t;
  t.x = x;
  t.y = y;
  t.tileType = tileType;
  t.biomeType = biomeType;
  t.terrainType = terrainType;
  mtx.lock();
  terrainMap[z][{ x, y }] = t;
  objectMap[z].erase({ x, y });
  mtx.unlock();
}

void MapController::processChunk(SDL_Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
  );

  for (auto h = 0; h < maxDepth; h++)
  {
    for (auto i = chunkRect->x; i != chunkRect->w; i++)
    {
      for (auto j = chunkRect->y; j != chunkRect->h; j++)
      {
        f(h, i ,j);
      }
    }
  }

}


void MapController::iterateOverChunk(SDL_Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
  );

  Rect r { chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h };
  auto rects = r.getRects();
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Analyzing chunk: (x1: %d, y1: %d) (x2: %d, y2: %d) [%d]", chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h, static_cast<int>(rects->size()));
  auto it = rects->begin();
  while (it != rects->end())
  {
    std::thread t([this, f](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < maxDepth; h++)
      {
        for (auto i = x1; i != x2; i++)
        {
          for (auto j = y1; j != y2; j++)
          {
            f(h, i, j);
          }
        }
      }
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "\t- (x1: %d, y1: %d) (x2: %d, y2: %d)", it->x1, it->x2, it->y1, it->y2);
    ++it;
  }
}


std::map<int, std::vector<SDL_Point>> MapController::getAllPointsInRect(SDL_Rect* r)
{
  std::map<int, std::vector<SDL_Point>> results;
  for (auto h = 0; h < maxDepth; h++)
  {
    std::vector<SDL_Point> points;
    for (auto i = r->x; i != r->w; i++)
    {
      for (auto j = r->y; j != r->h; j++)
      {
        SDL_Point p = { i, j };
        points.push_back(p);
      }
    }
    results[h] = points;
  }
  return results;
}


std::map<int, std::map<std::string, int>> MapController::getTilesInRange (SDL_Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> tilesInRange;
  auto lambda = [this, &tilesInRange](int h, int i, int j)
  {
    if (terrainMap[h].find({ i, j }) != terrainMap[h].end())
    {
      tilesInRange[h][terrainMap[h][{i, j}].tileType->name] += 1;
    }
  };
  processChunk(rangeRect, lambda);
  return tilesInRange;
}


std::map<int, std::map<std::string, std::map<std::string, int>>> MapController::getCountsInRange (SDL_Rect* r)
{
  std::map<int, std::map<std::string, std::map<std::string, int>>> res;
  auto lambda = [this, &res](int h, int i, int j)
  {
    auto it = terrainMap[h].find({ i, j });
    if (it != terrainMap[h].end())
    {
      res[h]["terrain"][it->second.terrainType->name]++;
      res[h]["biome"][it->second.biomeType->name]++;
    }
  };
  processChunk(r, lambda);
  return res;
}


std::map<int, std::map<std::string, int>> MapController::getBiomesInRange (SDL_Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> results;
  auto lambda = [this, &results](int h, int i, int j)
  {
    if (terrainMap[h].find({ i, j }) != terrainMap[h].end())
    {
      results[h][terrainMap[h][{i, j}].biomeType->name] += 1;
    }
  };
  processChunk(rangeRect, lambda);
  return results;
}


void MapController::randomlyAccessAllTilesInChunk(SDL_Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
  );
  auto coordinates = getAllPointsInRect(chunkRect);
  for (auto h = 0; h < maxDepth; h++)
  {
    while (coordinates[h].size())
    {
      int i = std::rand() % coordinates[h].size();
      SDL_Point p = coordinates[h].at(i);
      f(h, p.x, p.y);
      coordinates[h].erase(coordinates[h].begin() + i);
    }
  }
}


// TODO: Parallelize this; e.g., lambdas should operate on minichunks of larger chunks, etc.

// ALMOST THERE: remove/lock all stuff that might be accessed by multiple threads. including "mapGenerator"
int MapController::generateMapChunk(SDL_Rect* chunkRect)
{

  SDL_Log("Generating chunk."); //Current biome: %s", mapGenerator.currentBiomeType->name.c_str());

  auto createTerrainObjects = [this](int h, int i, int j)
  {
    auto b = &biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]];
    auto it = terrainMap[h].find({i, j});
    if (it == terrainMap[h].end())
    {
      auto randomType = b->terrainTypes[std::rand() % b->terrainTypes.size()].first;
      updateTile(h, i, j, b, &tileTypes[randomType], &terrainTypes[randomType]);
    }
  };

  auto fudgeBiomes = [this](int h, int i, int j)
  {
    auto it = terrainMap[h].find({i, j});
    if (it != terrainMap[h].end() && it->second.seen != true)
    {
      SDL_Rect r = { it->second.x-3, it->second.y-3, it->second.x+3, it->second.y+3 };
      auto results = getCountsInRange(&r);
      if (it->second.biomeType->name == "wasteland" && results[h]["biome"]["snowlands"] > 2)
      {
        updateTile(h, i, j, &biomeTypes["snow"], &tileTypes["snow"], &terrainTypes["snow"]);
      }
      if (results[h]["biome"]["water"] > 2)
      {
        updateTile(h, i, j, &biomeTypes["water"], &tileTypes["water"], &terrainTypes["water"]);
      }
    }
  };

  auto addWorldObjects = [this](int h, int i, int j)
  {
    auto it = terrainMap[h].find({i, j});
    if (it != terrainMap[h].end() && it->second.seen != true)
    {
      int layer = 0;
      for (auto relatedObjectType : it->second.terrainType->objects)
      {
        int threshold = 1000;
        if (!objectTypes[relatedObjectType].biomes[it->second.biomeType->name])
        {
          continue;
        }
        if (std::rand() % 1000 > 825)
        {
          std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
            i, j, &objectTypes[relatedObjectType], &tileTypes[relatedObjectType]
          );
          objectMap[h][{o->x, o->y}][layer] = o;
          layer++;
        }
      }
    }
  };

  iterateOverChunk(chunkRect, createTerrainObjects);
  randomlyAccessAllTilesInChunk(chunkRect, fudgeBiomes);
  processChunk(chunkRect, addWorldObjects);

  SDL_Log("Created chunk. Map now has %lu tiles", terrainMap[0].size());
  return 0;
}