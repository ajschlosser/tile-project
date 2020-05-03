#include "map.h"

std::shared_mutex mtx;

BiomeType* MapController::updateTile (int z, int x, int y, BiomeType* biomeType, TerrainType* terrainType, objects::objectsVector worldObjects = objects::objectsVector ())
{
  if (!cfg->biomeExistsOnLevel(biomeType->name, z))
  {
    biomeType = cfg->getRandomBiomeType(z);
    terrainType = &cfg->terrainTypes[biomeType->getRandomTerrainTypeName()];
  }
  std::unique_lock lock(mtx);
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

std::vector<std::shared_ptr<MobObject>>::iterator MapController::moveMob (std::string id, std::tuple<int, int, int> origin, std::tuple<int, int, int> destination)
{
  auto [z1, x1, y1] = origin;
  auto [z2, x2, y2] = destination;

  if (!isPassable(destination))
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
  int n = std::rand() % 10;
  for (auto h = 0; h < maxDepth; h++)
    for (auto i = n > 5 ? chunkRect->x1 : chunkRect->x2; [i,chunkRect,n](){if(n>5)return i<=chunkRect->x2;else return i>=chunkRect->x1;}() ; [&i,n](){if(n>5)i+=1;else i-=1;}())
      for (auto j = n > 5 ? chunkRect->y1 : chunkRect->y2; [j,chunkRect,n](){if(n>5)return j<=chunkRect->y2;else return j>=chunkRect->y1;}() ; [&j,n](){if(n>5)j+=1;else j-=1;}())
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
    std::thread t([this, functors](int x1, int y1, int x2, int y2) {
      for (auto h = 0; h < maxDepth; h++)
      {
        BiomeType* b = cfg->getRandomBiomeType(h);
        for (auto i = x1; i != x2; i++)
          for (auto j = y1; j != y2; j++)
            for (auto f : functors)
              f(h, i, j, b);
      }
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
  std::unique_lock lock(mtx);
  auto t = chunk::getRangeReport([this, h](int x, int y, chunk::ChunkReport* r){
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
            i, j, &cfg->objectTypes[n], &cfg->biomeTypes[b->name]
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


  auto addWorldObjects = [this](int h, int i, int j, BiomeType* b)
  {
    auto it = terrainMap[h].find({i, j});
    if (it != terrainMap[h].end() && it->second.initialized == false)
    {
      if (std::rand() % 1000 > 995)
      {
        
        for (auto mob = cfg->mobTypes.begin(); mob != cfg->mobTypes.end(); mob++)
        {
          if (mob->second.biomes.find(it->second.biomeType->name) != mob->second.biomes.end())
          {
            std::shared_ptr<MobObject> m = std::make_shared<MobObject>(
              i, j, &mob->second, &cfg->biomeTypes[it->second.biomeType->name]
            );

            if (mob->second.isAnimated())
            {

              // m->simulators.push_back(std::make_shared<simulated::Simulator<MobObject>>(m,[this,h,i,j,m](std::shared_ptr<MobObject> mo){

                
              //   if (std::rand() % 2 > 1) m->x = std::rand() % 2 > 1 ? m->x + 1 : m->x - 1;
              //   if (std::rand() % 2 > 1) m->y = std::rand() % 2 > 1 ? m->y + 1 : m->y - 1;
              //   // TODO change x,y here, have engine check for diff and then use moveMob on diff YO

              // }));

              m->animationTimer.start();
              m->animationSpeed = mob->second.animationSpeed + std::rand() % 3000;
            }

            updateTile(h, i, j, nullptr, m);
          }
        }
      }
      std::unique_lock lock(mtx);
      it->second.initialized = true;
    }
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

  chunk::multiprocessFunctorVec terrainPlacement { { createTerrainObjects, [this](chunk::ChunkProcessor* p, int z, std::tuple<int, int> coords)
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
  chunk::multiprocessFunctorVec chunkFudging {
    { fudgeChunk, [this](chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { hammerChunk, [this](chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { fudgeChunk, [this](chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} },
    { cleanChunk, [this](chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(z);} }
  };
  chunk::multiprocessFunctorVec objectPlacement { { addWorldObjects, [this](chunk::ChunkProcessor* p,int z,std::tuple<int,int>coords){return cfg->getRandomBiomeType(); } } };

  chunk::ChunkProcessor chunker ( chunkRect, maxDepth );
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