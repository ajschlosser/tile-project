#include "map.h"

void MapController::iterateOverChunk(SDL_Rect* chunkRect, std::function<void(int, int, int)> f)
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
  iterateOverChunk(rangeRect, lambda);
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
  iterateOverChunk(r, lambda);
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
  iterateOverChunk(rangeRect, lambda);
  return results;
}


// TODO: If this takes too long, bad things happen
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


int MapController::generateMapChunk(SDL_Rect* chunkRect)
{
  if (mapGenerator.currentlyGenerating())
  {
    SDL_Log("already generating; bailing");
    return -1;
  }

  mapGenerator.init(&biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]]);
  SDL_Log("Generating chunk. Current biome: %s", mapGenerator.currentBiomeType->name.c_str());

  auto createTerrainObjects = [this](int h, int i, int j)
  {
    while (mapGenerator.isOutOfDepth(h))
    {
      SDL_Log("Can't generate biome %s on level %d", mapGenerator.currentBiomeType->name.c_str(), h);
      mapGenerator.currentBiomeType = &biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]];
      SDL_Log("Trying biome: %s", mapGenerator.currentBiomeType->name.c_str());
    }
    auto b = mapGenerator.currentBiomeType;
    auto it = terrainMap[h].find({i, j});
    if (it == terrainMap[h].end())
    {
      auto randomType = b->terrainTypes[std::rand() % b->terrainTypes.size()].first;
      TerrainObject t { i, j, b, &terrainTypes[randomType], &tileTypes[randomType] };
      terrainMap[h][{i, j}] = t;
    } //else { SDL_Log("shit: %s", it->second.terrainType->name.c_str()); }
  };

  auto fudgeBiomes = [this](int h, int i, int j)
  {
    auto it = terrainMap[h].find({i, j});
    if (it != terrainMap[h].end() && it->second.seen != true)
    {
      SDL_Rect r = { it->second.x-3, it->second.y-3, it->second.x+3, it->second.y+3 };
      auto results = getCountsInRange(&r);
      if (results[h]["biome"]["water"] > 2) {
        TerrainObject t { i, j, &biomeTypes["water"], &terrainTypes["water"], &tileTypes["water"] };
        terrainMap[h][{i, j}] = t;
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
  iterateOverChunk(chunkRect, addWorldObjects);

  mapGenerator.reset();
  SDL_Log("Created chunk. Map now has %lu tiles", terrainMap[0].size());
  return 0;
}