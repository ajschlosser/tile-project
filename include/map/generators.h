#ifndef GAME_MAP_GENERATORS_H
#define GAME_MAP_GENERATORS_H

#include "map.h"

using namespace map;

int MapController::generateMapChunk(Rect* chunkRect)
{
  if (mapGenerator.processing)
  {
    std::unique_lock lock(mtx);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Already processing chunk.");
    return -1;
  }

  mapGenerator.init(&mtx);

  auto createTerrainObjects = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (it == terrainMap[h].end())
    {
      TerrainType* tt;
      Rect range = { i-1, j-1, i+1, j+1 };
      auto t = generateRangeReport(&range, h);
      auto [tCount, topTerrainName] = t.topTerrain[h];
      auto [bCount, topBiomeName] = t.topBiome[h];
      if (topTerrainName.length() > 0 && cfg->terrainTypes[topTerrainName].clusters == true && cfg->biomeExistsOnLevel(topBiomeName, h))
      {
        tt = &cfg->terrainTypes[topTerrainName];
        b = &cfg->biomeTypes[topBiomeName]; 
      }
      else
        tt = &cfg->terrainTypes[b->getRandomTerrainTypeName()];
      b = updateTile(h, i, j, b, tt);
      if ((std::rand() % 10000 > (9500 - ((9500 * tt->getObjectFrequencyMultiplier()) - 9500))) && tt->objectTypeProbabilities.size() > 0)
      {
        std::string n = tt->getRandomObjectTypeName(); // TODO: First check if it's possible, then keep checking until you've got it
        if (cfg->objectTypes[n].biomes[b->name] && worldMap[h][{i, j}].begin() == worldMap[h][{i, j}].end())
        {
          std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
            i, j, h, &cfg->objectTypes[n], &cfg->biomeTypes[b->name]
          );
          if (cfg->objectTypes[n].isAnimated())
          {
            o->animationTimer.start();
            o->animationSpeed = cfg->objectTypes[n].animationSpeed + std::rand() % 3000;
          }
          updateTile(h, i, j, o, nullptr);
        }  
      }
    }
  };


  auto addMobs = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (isPassable({h, i, j}) && it != terrainMap[h].end() && it->second.initialized == false)
    {
      if (std::rand() % 1000 > 975)
      {
        
        for (auto mob = cfg->mobTypes.begin(); mob != cfg->mobTypes.end(); mob++)
        {
          if (mob->second.biomes.find(it->second.biomeType->name) != mob->second.biomes.end())
          {
            std::shared_ptr<MobObject> m = std::make_shared<MobObject>(
              i, j, h, &mob->second, &cfg->biomeTypes[it->second.biomeType->name]
            );

            if (mob->second.isAnimated())
            {

              m->simulators.push_back(std::make_shared<simulated::Simulator<MobObject>>(
                [this,h,i,j,m]()
                {
                  
                  int n = std::rand() % 100;
                  if (n > 50)
                    m->x += std::rand() % 100 > 50 ? 1 : -1;
                  else
                    m->y += std::rand() % 100 > 50 ? 1 : -1;
                  if (isPassable({h, i, j}))
                    m->orders += simulated::MOVE;
                }
              ));

              m->animationTimer.start();
              m->animationSpeed = mob->second.animationSpeed + std::rand() % 3000;
            }

            updateTile(h, i, j, nullptr, m);
          }
        }
      }
    }
    std::unique_lock lock(mtx);
    it->second.initialized = true;
  };

  auto hammerChunk = [this](Rect* r, BiomeType* b)
  {
    auto hammerProcessor = [this](int h, int i, int j)
    {
      auto it = terrainMap[h].find({ i, j });
      if (it != terrainMap[h].end() && it->second.initialized == false)
      {
        Rect range = { i-3, j-3, i+3, j+3 };
        auto t = generateRangeReport(&range, h);
        auto [bCount, topBiomeName] = t.topBiome[h];
        if (std::rand() % 1000 > 985) topBiomeName = cfg->getRandomBiomeType(h)->name;
        if (terrainMap[h].find({ i, j })->second.initialized == false) updateTile(h, i, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i+1, j })->second.initialized == false) updateTile(h, i+1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i-1, j })->second.initialized == false) updateTile(h, i-1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i, j+1 })->second.initialized == false) updateTile(h, i, j+1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
        if (terrainMap[h].find({ i, j-1 })->second.initialized == false) updateTile(h, i, j-1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
      }
    };
    processChunk(r, hammerProcessor);
  };

  auto cleanChunk = [this](Rect* r, BiomeType* b)
  {
    auto processor = [this](int h, int i, int j)
    {
      auto it = terrainMap[h].find({ i, j });
      if (it != terrainMap[h].end() && it->second.initialized == false)
      {
        Rect range = { i-3, j-3, i+3, j+3 };
        auto t = generateRangeReport(&range, h);
        auto [bCount, topBiomeName] = t.topBiome[h];
        if (t.biomeCounts[h][it->second.biomeType->name] <= 2) updateTile(h, i, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
      }
    };
    processChunk(r, processor);
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
      else if (it->second.initialized == false)
      {
        Rect range = { i-2, j-2, i+2, j+2 };
        auto t = generateRangeReport(&range, h);
        auto [bCount, topBiomeName] = t.topBiome[h];
        if (it->second.biomeType->name != topBiomeName && cfg->biomeExistsOnLevel(topBiomeName, h))
        {
            if (std::rand() % 10 > 4)
            {
              updateTile(h, i, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
            }
            else
            {
              updateTile(h, i+1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
              updateTile(h, i-1, j, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
              updateTile(h, i, j+1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
              updateTile(h, i, j-1, &cfg->biomeTypes[topBiomeName], cfg->getRandomTerrainType(topBiomeName) );
            }
        }
      }
    };
    if (std::rand() % 100 > 65) processChunk(r, fudgeProcessor);
  };

  map::chunk::multiprocessFunctorVec terrainPlacement { { createTerrainObjects, [this](map::chunk::ChunkProcessor* p, int z, std::tuple<int, int> coords)
  {
    if (cfg->biomeExistsOnLevel(p->getBrush()->name, z) == false)
      p->setBrush(cfg->getRandomBiomeType(z)); 
    if (std::rand() % 10 > 5)
    {
      auto [i, j] = coords;
      Rect range = { i-5, j-5, i+5, j+5 };
      auto t = generateRangeReport(&range, 0);
      auto [bCount, topBiomeName] = t.topBiome[z];
      if (topBiomeName.length() > 0 && cfg->biomeExistsOnLevel(topBiomeName, z) == true)
        p->setBrush(&cfg->biomeTypes[topBiomeName]);
      else
        p->setBrush(cfg->getRandomBiomeType(z));
    }
    return p->getBrush(); } }

  };
  map::chunk::multiprocessFunctorVec chunkFudging {
    { fudgeChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { hammerChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { fudgeChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { cleanChunk, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} }
  };
  map::chunk::multiprocessFunctorVec objectPlacement { { addMobs, [this](map::chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(); } } };

  map::chunk::ChunkProcessor chunker ( chunkRect, maxDepth );
  chunker.setBrush(cfg->getRandomBiomeType(0));
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

#endif