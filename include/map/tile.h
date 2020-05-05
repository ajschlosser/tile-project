#ifndef GAME_MAP_TILE_H
#define GAME_MAP_TILE_H

#include "map.h"

using namespace map;

std::shared_mutex tileMutex;

BiomeType* MapController::updateTile (int z, int x, int y, BiomeType* biomeType, TerrainType* terrainType, objects::objectsVector worldObjects = objects::objectsVector ())
{
  if (!cfg->biomeExistsOnLevel(biomeType->name, z))
  {
    biomeType = cfg->getRandomBiomeType(z);
    terrainType = &cfg->terrainTypes[biomeType->getRandomTerrainTypeName()];
  }
  std::unique_lock lock(tileMutex);
  TerrainObject t;
  t.x = x;
  t.y = y;
  t.biomeType = biomeType;
  t.terrainType = terrainType;
  if (terrainType->isAnimated())
  {
    t.animationTimer.start();
    t.animationSpeed = terrainType->animationSpeed + std::rand() % 3000;
  }
  terrainMap[z][{ x, y }] = t;


  for (auto o : worldMap[z][{ x, y }])
    if (!o->objectType->biomes[biomeType->name])
    {
      worldMap[z][{ x, y }].clear();
      break;
    }
  
  
  mobMap[z][{ x, y }].clear();
  BiomeObject b;
  b.biomeType = biomeType;
  b.x = x;
  b.y = y;
  biomeMap[z][{ x, y }] = b;
  return biomeType;
}

void MapController::updateTile (int z, int x, int y, std::shared_ptr<WorldObject> w = nullptr, std::shared_ptr<MobObject> m = nullptr)
{
  std::unique_lock lock(tileMutex);
  if (w != nullptr)
  {
    worldMap[z][{x, y}].push_back(w);
  }
  if (m != nullptr)
  {
    mobMap[z][{x, y}].push_back(std::move(m));
  }
}

bool MapController::isPassable (std::tuple<int, int, int> coords)
{
  auto [_z, _x, _y] = coords;
  auto terrainIt = terrainMap[_z].find({ _x, _y });
  if (terrainIt == terrainMap[_z].end())
    return false;
  else if (terrainIt->second.terrainType->impassable)
    return false;
  auto worldIt = worldMap[_z].find({ _x, _y });
  if (worldIt != worldMap[_z].end())
    for (auto o : worldIt->second)
      if (o->objectType->impassable)
        return false;
  auto mobIt = mobMap[_z].find({ _x, _y });
  if (mobIt != mobMap[_z].end())
    for (auto o : mobIt->second)
      if (o->mobType->impassable)
        return false;
  return true;
}

#endif