#ifndef GAME_MAP_UTIL_H
#define GAME_MAP_UTIL_H

#include "../map.h"

using namespace map;

std::shared_mutex mtx;

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


map::chunk::ChunkReport MapController::generateRangeReport(Rect* range, int h = 0)
{
  std::unique_lock lock(mtx);
  auto t = map::chunk::getRangeReport([this, h](int x, int y, map::chunk::ChunkReport* r){
    auto it = terrainMap[h].find({x, y});
    if (it != terrainMap[h].end())
    {
      r->terrainCounts[h][it->second.terrainType->name]++;
      auto [ topTerrainCount, topTerrainName ] = r->topTerrain[h];
      if (r->terrainCounts[h][it->second.terrainType->name] > topTerrainCount)
      {
        r->meta[h]["secondTopTerrain"] = topTerrainName;
        r->topTerrain[h] = { r->terrainCounts[h][it->second.terrainType->name], it->second.terrainType->name };
      }
      if (cfg->biomeExistsOnLevel(it->second.biomeType->name, h) == true)
      {
        r->biomeCounts[h][it->second.biomeType->name]++;
        auto [ topBiomeCount, topBiomeName ] = r->topBiome[h];
        if (r->biomeCounts[h][it->second.biomeType->name] > topBiomeCount)
        {
          r->meta[h]["secondTopBiome"] = topBiomeCount;
          r->topBiome[h] = { r->biomeCounts[h][it->second.biomeType->name], it->second.biomeType->name };
        }
      }
    }
  }, range, 1, 1);
  return t;
}

#endif