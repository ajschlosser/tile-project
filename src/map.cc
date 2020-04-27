#include "map.h"

std::shared_mutex mtx;

std::shared_mutex* map::getMutex ()
{
  return &mtx;
}

void MapController::updateTile (int z, int x, int y, BiomeType* biomeType, TerrainType* terrainType, objects::objectsVector worldObjects = objects::objectsVector ())
{
  std::unique_lock lock(mtx);
  TerrainObject t;
  t.x = x;
  t.y = y;
  t.biomeType = biomeType;
  t.terrainType = terrainType;
  terrainMap[z][{ x, y }] = t;
  BiomeObject b;
  b.biomeType = biomeType;
  b.x = x;
  b.y = y;
  terrainMap[z][{ x, y }] = t;
  biomeMap[z][{ x, y }] = b;
}

void MapController::updateTile (int z, int x, int y, std::shared_ptr<WorldObject> w = nullptr, std::shared_ptr<MobObject> m = nullptr)
{
  std::unique_lock lock(mtx);
  if (w != nullptr)
  {
    worldMap[z][{x, y}].push_back(w);
  }
  if (m != nullptr)
  {
    mobMap[z][{x, y}].push_back(std::move(m));
  }
}

std::vector<std::shared_ptr<MobObject>>::iterator MapController::moveMob (std::string id, std::tuple<int, int, int> origin, std::tuple<int, int, int> destination)
{
  auto [z1, x1, y1] = origin;
  auto [z2, x2, y2] = destination;

  auto terrainIt = terrainMap[z2].find({ x2, y2 });
  if (terrainIt == terrainMap[z2].end())
    return mobMap[z1][{x1, y1}].begin();
  else if (terrainIt->second.terrainType->impassable)
    return mobMap[z1][{x1, y1}].begin();

  std::unique_lock lock(mtx);
  auto it = mobMap[z1][{x1, y1}].begin();
  while (it != mobMap[z1][{x1, y1}].end())
  {
    if (it->get()->id == id)
    {
      mobMap[z2][{x2, y2}].push_back((*it));
      it = mobMap[z1][{x1, y1}].erase(it);
      return it;
    }
    else
      ++it;
  }
  return it;
}

void MapController::processChunk(Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2
  );
  for (auto h = 0; h < maxDepth; h++)
    for (auto i = chunkRect->x1; i < chunkRect->x2; i++)
      for (auto j = chunkRect->y1; j < chunkRect->y2; j++)
        f(h, i ,j);
}

template <typename F>
void MapController::iterateOverChunk(Rect* chunkRect, F functors)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2
  );

  Rect r { chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2 };
  auto rects = r.getRects();
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Analyzing chunk: (x1: %d, y1: %d) (x2: %d, y2: %d) [%d]", chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2, static_cast<int>(rects->size()));
  auto it = rects->begin();
  while (it != rects->end())
  {
    BiomeType* b = getRandomBiomeType();
    std::thread t([this, functors, b](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < maxDepth; h++)
        for (auto i = x1; i != x2; i++)
          for (auto j = y1; j != y2; j++)
            for (auto f : functors) f(h, i, j, b);
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "\t- (x1: %d, y1: %d) (x2: %d, y2: %d)", it->x1, it->x2, it->y1, it->y2);
    ++it;
  }
}

template <typename F>
void MapController::iterateOverChunkEdges(Rect* chunk, F f)
{
  Rect top { chunk->x1, chunk->y1, chunk->x2, chunk->y1 };
  Rect bottom { chunk->x1, chunk->y2, chunk->x2, chunk->y2 };
  Rect left { chunk->x1, chunk->y1, chunk->x1, chunk->y2 };
  Rect right { chunk->x2, chunk->y1, chunk->x2, chunk->y2 };
  std::vector<Rect> edges { top, bottom, left, right };
  for (auto it = edges.begin(); it != edges.end(); ++it)
  {
    std::thread t([this, f](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < maxDepth; h++)
        for (auto i = x1; i <= x2; i++)
          for (auto j = y1; j <= y2; j++)
            f(h, i, j);
    }, it->x1, it->y1, it->x2, it->y2);
    t.detach();
  }
}


std::map<int, std::vector<SDL_Point>> MapController::getAllPointsInRect(Rect* r)
{
  std::map<int, std::vector<SDL_Point>> results;
  for (auto h = 0; h < maxDepth; h++)
  {
    std::vector<SDL_Point> points;
    for (auto i = r->x1; i != r->x2; i++)
    {
      for (auto j = r->y1; j != r->y2; j++)
      {
        SDL_Point p = { i, j };
        points.push_back(p);
      }
    }
    results[h] = points;
  }
  return results;
}


std::map<int, std::map<std::string, int>> MapController::getTilesInRange (Rect* r)
{
  std::map<int, std::map<std::string, int>> t;
  auto lambda = [this, &t](int h, int i, int j)
  {
    if (terrainMap[h].find({ i, j }) != terrainMap[h].end())
      t[h][terrainMap[h][{i, j}].terrainType->name] += 1;
  };
  processChunk(r, lambda);
  return t;
}


std::map<int, std::map<std::string, std::map<std::string, int>>> MapController::getCountsInRange (Rect* r)
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



std::map<int, std::map<std::string, int>> MapController::getBiomesInRange (Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> results;
  auto lambda = [this, &results](int h, int i, int j)
  {
    if (terrainMap[h].find({ i, j }) != terrainMap[h].end())
      results[h][terrainMap[h][{i, j}].biomeType->name] += 1;
  };
  processChunk(rangeRect, lambda);
  return results;
}


void MapController::randomlyAccessAllTilesInChunk(Rect* chunkRect, std::function<void(int, int, int)> f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    maxDepth, chunkRect->x1, chunkRect->y1, chunkRect->x2, chunkRect->y2
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

chunk::ChunkReport MapController::generateRangeReport(Rect* range, int h = 0)
{
  auto t = chunk::getRangeReport([this, h](int x, int y, chunk::ChunkReport* r){
    auto it = terrainMap[h].find({x, y});
    if (it != terrainMap[h].end())
    {
      r->terrainCounts[h][it->second.terrainType->name]++;
      r->biomeCounts[h][it->second.biomeType->name]++;
      auto [ topTerrainCount, topTerrainName ] = r->topTerrain;
      auto [ topBiomeCount, topBiomeName ] = r->topBiome;
      if (r->terrainCounts[h][it->second.terrainType->name] > topTerrainCount)
      {
        r->meta["secondTopTerrain"] = topTerrainName;
        r->topTerrain = { r->terrainCounts[h][it->second.terrainType->name], it->second.terrainType->name };
      }
      if (r->biomeCounts[h][it->second.biomeType->name] > topBiomeCount)
      {
        r->meta["secondTopBiome"] = topBiomeCount;
        r->topBiome = { r->biomeCounts[h][it->second.biomeType->name], it->second.biomeType->name };
      }
    }
  }, range, 1, 1);
  return t;
}

int MapController::generateMapChunk(Rect* chunkRect)
{
  if (mapGenerator.processing)
  {
    SDL_Log("Already processing chunk.");
    return -1;
  }

  mapGenerator.init(getRandomBiomeType(), &mtx);

  auto createTerrainObjects = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (it == terrainMap[h].end())
    {
      auto randomTerrainType = b->terrainTypes[std::rand() % b->terrainTypes.size()].first;
      updateTile(h, i, j, b, &cfg->terrainTypes[randomTerrainType]);
    }
  };


  auto addWorldObjects = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (it != terrainMap[h].end() && it->second.initialized == false)
    {
      for (auto relatedObjectType : it->second.terrainType->objects)
      {
        if (!cfg->objectTypes[relatedObjectType].biomes[it->second.biomeType->name])
          continue;
        if (std::rand() % 1000 > 825)
        {
          std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
            i, j, &cfg->objectTypes[relatedObjectType], &cfg->biomeTypes[it->second.biomeType->name]
          );
          updateTile(h, i, j, o, nullptr);
        }
      }
      if (std::rand() % 1000 > 900)
      {
        for ( auto mob : cfg->mobTypes )
        {
          if (mob.second.biomes.find(it->second.biomeType->name) != mob.second.biomes.end())
          {
            std::shared_ptr<MobObject> m = std::make_shared<MobObject>(
              i, j, mob.second, &cfg->biomeTypes[it->second.biomeType->name]
            );
            updateTile(h, i, j, nullptr, m);
          }
        }
      }
      std::unique_lock lock(mtx);
      it->second.initialized = true;
    }
  };

  auto fudgeChunk = [this](Rect* r, BiomeType* b)
  {
    auto fudgeProcessor = [this](int h, int i, int j)
    {
      auto it = terrainMap[h].find({ i, j });
      if (it == terrainMap[h].end())
      {
        //make tile based on most common biome in range of 1
      }
      else if (!it->second.initialized)
      {
        Rect range = { i-2, j-2, i+2, j+2 };

        auto t = generateRangeReport(&range, h);

        auto [bCount, bName] = t.topBiome;
        //SDL_Log("%s %d %s %s %s", it->second.biomeType->name.c_str(), bCount, bName.c_str(), cfg->biomeTypes[bName].name.c_str(), cfg->getRandomTerrainType(bName)->name.c_str() );

        if (it->second.biomeType->name != bName)
          updateTile(h, i, j, &cfg->biomeTypes[bName], cfg->getRandomTerrainType(bName) );
        else
        {
          if (t.biomeCounts[h][it->second.biomeType->name] < 3)
          {
            updateTile(h, i, j, &cfg->biomeTypes[bName], cfg->getRandomTerrainType(bName) );
          }
        }
      }
    };

    int n = std::rand() % 100;
    if (n > 85) iterateOverChunkEdges(r, fudgeProcessor);
    else if (n > 65 ) processChunk(r, fudgeProcessor);
    else randomlyAccessAllTilesInChunk(r, fudgeProcessor);
  };

  typedef std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>> multiprocessChain;
  multiprocessChain terrainPlacement { { createTerrainObjects, [this](){return getRandomBiomeType();} } };
  multiprocessChain chunkFudging {
    { fudgeChunk, [this](){return getRandomBiomeType();} },
    { fudgeChunk, [this](){return getRandomBiomeType();} },
    { fudgeChunk, [this](){return getRandomBiomeType();} },
    { fudgeChunk, [this](){return getRandomBiomeType();} },
    { fudgeChunk, [this](){return getRandomBiomeType();} }
  };
  multiprocessChain objectPlacement { { addWorldObjects, [this](){return getRandomBiomeType(); } } };

  chunk::ChunkProcessor chunker ( chunkRect, maxDepth );
  SDL_Log("Adding terrain objects...");
  // std::thread t([this, &chunker](multiprocessChain o, multiprocessChain c){ chunker.multiProcessChunk({ o, c }); }, objectPlacers, chunkFuzzers);
  // t.join();
  chunker.multiProcessChunk({ terrainPlacement, chunkFudging });
  SDL_Log("Adding world and mob objects...");
  
  // TODO: Chunkfuzz is fine but not in this case because this is what initializes all tiles. Need something else for that
  chunker.multiProcessChunk({ objectPlacement });
  SDL_Log("Done adding objects.");

  mapGenerator.reset(&mtx);
  std::unique_lock lock(mtx);
  SDL_Log("Created chunk. Map now has %lu terrain objects, %lu world objects, and %lu mob objects for a total of %lu",
    terrainMap[0].size()*2,
    worldMap[0].size()*2,
    terrainMap[0].size()*2,
    terrainMap[0].size()*2+worldMap[0].size()*2+mobMap[0].size()*2
  );
  return 0;
}