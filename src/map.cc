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
  TileObject tile;
  tile.x = x;
  tile.y = y;
  tile.setTerrainObject(t);
  tileMap[z][{ x, y }] = tile;
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

  std::unique_lock lock(mtx);
  auto it = mobMap[z1][{x1, y1}].begin();
  while (it != mobMap[z1][{x1, y1}].end())
  {
    if (it->get()->id == id)
    {
      //SDL_Log("s %s", id.c_str());
      mobMap[z2][{x2, y2}].push_back((*it));
      //SDL_Log("erasing %s", it->get()->id.c_str());
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
    for (auto i = chunkRect->x1; i != chunkRect->x2; i++)
      for (auto j = chunkRect->y1; j != chunkRect->y2; j++)
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
    if (tileMap[h].find({ i, j }) != tileMap[h].end())
      t[h][tileMap[h][{i, j}].getTerrainType()->name] += 1;
  };
  processChunk(r, lambda);
  return t;
}


ChunkReport MapController::getChunkReport (Rect* r)
{
  ChunkReport report;

  auto lambda = [this, &report](int h, int i, int j)
  {
    std::map<std::string, std::tuple<std::string, int>> top;
    top["biome"] = {"none", 0};
    top["terrain"] = {"none", 0};
    auto it = tileMap[h].find({ i, j });
    if (it != tileMap[h].end() && it->second.initialized == true)
    {
      report.counts[h]["terrain"][it->second.terrainType->name]++;
      report.counts[h]["biome"][it->second.getBiomeType()->name]++;

      int bn = report.counts[h]["biome"][it->second.getBiomeType()->name];
      int tn = report.counts[h]["terrain"][it->second.terrainType->name];

      auto [ topBiomeName, topBiomeCount ] = top["biome"];
      auto [ topTerrainName, topTerrainCount ] = top["terrain"];

      if (bn > topBiomeCount)
      {
        top["biome"] = { it->second.getBiomeType()->name, bn };
        report.top[h]["biome"] = it->second.getBiomeType()->name;
      }
      if (tn > topTerrainCount)
      {
        top["terrain"] = { it->second.terrainType->name, bn };
        report.top[h]["terrain"] = it->second.terrainType->name;
      }

    }
  };
  processChunk(r, lambda);
  return report;
}


std::map<int, std::map<std::string, std::map<std::string, int>>> MapController::getCountsInRange (Rect* r)
{
  std::map<int, std::map<std::string, std::map<std::string, int>>> res;
  auto lambda = [this, &res](int h, int i, int j)
  {
    auto it = tileMap[h].find({ i, j });
    if (it != tileMap[h].end())
    {
      res[h]["terrain"][it->second.getTerrainType()->name]++;
      res[h]["biome"][it->second.getBiomeType()->name]++;
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
    if (tileMap[h].find({ i, j }) != tileMap[h].end())
      results[h][tileMap[h][{i, j}].getBiomeType()->name] += 1;
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


int MapController::generateMapChunk(Rect* chunkRect)
{
  if (mapGenerator.processing)
  {
    SDL_Log("Already processing %s... Stopped.", mapGenerator.currentBiomeType->name.c_str());
    return -1;
  }

  mapGenerator.init(getRandomBiomeType(), &mtx);
  SDL_Log("Generating chunk. Current biome: %s", mapGenerator.currentBiomeType->name.c_str());

  auto createTerrainObjects = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = tileMap[h].find({i, j});
    if (it == tileMap[h].end())
    {
      auto randomTerrainType = b->terrainTypes[std::rand() % b->terrainTypes.size()].first;
      updateTile(h, i, j, b, &terrainTypes[randomTerrainType]);
    }
  };

  auto fudgeBiomes = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = tileMap[h].find({i, j});
    if (it != tileMap[h].end() && it->second.initialized == false) // && it->second.seen != true)
    {
      Rect r = { it->second.x-3, it->second.y-3, it->second.x+3, it->second.y+3 };
      auto results = getChunkReport(&r);

      if (it->second.getBiomeType()->name == "wasteland" && results.counts[h]["biome"]["snowlands"] > 2)
        updateTile(h, i, j, &biomeTypes["snow"], &terrainTypes["snow"]);
      else if (results.counts[h]["biome"]["water"] > 15)
        updateTile(h, i, j, &biomeTypes["water"], &terrainTypes["water"]);
    }
  };

  auto addWorldObjects = [this](int h, int i, int j)
  {
    auto it = tileMap[h].find({i, j});
    if (it != tileMap[h].end() && it->second.initialized == false) //&& it->second.seen != true)
    {
      for (auto relatedObjectType : it->second.getTerrainType()->objects)
      {
        if (!objectTypes[relatedObjectType].biomes[it->second.getBiomeType()->name])
        {
          continue;
        }
        if (std::rand() % 1000 > 825)
        {
          std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
            i, j, &objectTypes[relatedObjectType], &biomeTypes[it->second.getBiomeType()->name]
          );
          updateTile(h, i, j, o, nullptr);
        }
      }
      if (std::rand() % 1000 > 900)
      {
        for ( auto mob : mobTypes )
        {
          if (mob.second.biomes.find(it->second.getBiomeType()->name) != mob.second.biomes.end())
          {
            std::shared_ptr<MobObject> m = std::make_shared<MobObject>(
              i, j, mob.second, &biomeTypes[it->second.getBiomeType()->name]
            );
            updateTile(h, i, j, nullptr, m);
          }
        }
      }
      std::unique_lock lock(mtx);
      it->second.initialized = true;
    }
  };

  auto fuzzIt = [this](Rect* r, std::vector<chunkProcessorFunctor> v)
  {
    auto dingleTips = [this](int h, int i, int j)
    {
      updateTile(h, i, j, &biomeTypes["snowlands"], &terrainTypes["snow"] );
    };
    for (auto f : v) iterateOverChunkEdges(r, dingleTips);
  };

  Rect fudgeRect = { chunkRect->x1 - 15, chunkRect->y1 - 15, chunkRect->x2 + 15, chunkRect->y2 + 15}; // necessary to fudge edges of already processed chunks
  typedef std::vector<std::pair<genericChunkFunctor, std::function<BiomeType*()>>> multiprocessChain;
  multiprocessChain placers { { createTerrainObjects, [this](){return getRandomBiomeType();} } };
  multiprocessChain fuzzers { { fuzzIt, [this](){return &biomeTypes["water"];} } };
  std::vector<chunkFunctor> chonkers { addWorldObjects };

  ChunkProcessor chunker ( chunkRect, maxDepth );
  chunker.multiProcessChunk({ placers, fuzzers });
  chunker.processChunk({ chonkers });

  mapGenerator.reset(&mtx);
  std::unique_lock lock(mtx);
  SDL_Log("Created chunk. Map now has %lu tiles", tileMap[0].size()*2);
  return 0;
}