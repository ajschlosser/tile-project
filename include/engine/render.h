#ifndef GAME_ENGINE_RENDER_H
#define GAME_ENGINE_RENDER_H

#include "engine.h"

void GameEngine::renderCopyTiles()
{
  auto processor = [this](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto it = mapController.mobMap[zLevel][{i, j}].begin();
    while (it != mapController.mobMap[zLevel][{i, j}].end())
    {
      auto mob = it->get();
      for (auto s : mob->simulators)
        s->simulate();
      auto orders = it->get()->orders;
      if (orders & simulated::MOVE)
      {
        mob->orders -= simulated::MOVE;
        if (!mapController.isPassable(std::make_tuple(mob->z,mob->x,mob->y)))
          continue;
        auto direction = i != mob->x
          ? (i < mob->x ? tileObject::RIGHT : tileObject::LEFT)
          : j != mob->y
            ? (j < mob->y ? tileObject::DOWN : tileObject::UP)
            : tileObject::DOWN;

        it = mapController.moveMob(it, {zLevel, i, j}, direction);
        continue;
      }
      ++it;
    }
  };
  std::thread p (
    [this](std::function<void(std::tuple<int, int, int, int>)> f) { iterateOverTilesInView(f); }, processor
  );
  std::vector<std::pair<std::shared_ptr<MobObject>, std::tuple<int, int>>> movers;
  auto terrainRenderer = [this,&movers](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto terrainObject = mapController.terrainMap[zLevel].find({ i, j });
    if (terrainObject != mapController.terrainMap[zLevel].end())
      gfxController.renderCopyTerrain(&terrainObject->second, x, y);
    else
      gfxController.renderCopySprite("Sprite 0x128", x, y);
    auto worldObject = mapController.worldMap[zLevel].find({ i, j });
    if (worldObject != mapController.worldMap[zLevel].end())
      for ( auto w : worldObject->second )
        gfxController.renderCopyObject(w, x, y);
    auto mobObject = mapController.mobMap[zLevel].find({ i, j });
    // TODO: process differently depending on direction due to how tiles are drawn
    if (mobObject != mapController.mobMap[zLevel].end())
      for ( auto &w : mobObject->second )
      {
        if (w->relativeX == 0 && w->relativeY == 0)
        {
          gfxController.renderCopyMobObject(w, x, y);
        }
        else
        {
          movers.push_back({w, { x, y }});
          //gfxController.renderCopyMobObject(w, x, y);
        }
      }
  };
  // auto objectRenderer = [this](std::tuple<int, int, int, int> locationData){
  //   auto [x, y, i, j] = locationData;
  // };
  std::thread r (
    [this, &movers](std::function<void(std::tuple<int, int, int, int>)> f1)
    {
      iterateOverTilesInView(f1);
      auto it = movers.begin();
      while (it != movers.end())
      {
        auto mob = it->first;
        auto [_x, _y] = it->second;
        gfxController.renderCopyMobObject(mob, _x, _y);        
        movers.erase(it);
      }
    },
    terrainRenderer
  );
  p.join();
  r.join();
}


int GameEngine::renderCopyPlayer()
{
  player.x = gfxController.camera.x;
  player.y = gfxController.camera.y;
  auto [_w, _h] = gfxController.getWindowGridDimensions();
  SDL_Rect playerRect = { _w/2*tileSize, _h/2*tileSize, tileSize, tileSize };
  return SDL_RenderFillRect(appRenderer, &playerRect);
}

#endif