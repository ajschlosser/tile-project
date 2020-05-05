#include "engine/camera.h"
#include "engine/render.h"
#include "engine/graphics/render.h"

using namespace controller;

void RenderController::renderCopyTiles()
{
  auto processor = [this](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto it = e->mapController.mobMap[e->zLevel][{i, j}].begin();
    while (it != e->mapController.mobMap[e->zLevel][{i, j}].end())
    {
      auto mob = it->get();
      for (auto s : mob->simulators)
        s->simulate();
      auto orders = it->get()->orders;
      if (orders & simulated::MOVE)
      {
        mob->orders -= simulated::MOVE;
        if (!e->mapController.isPassable(std::make_tuple(mob->z,mob->x,mob->y)))
          continue;
        auto direction = i != mob->x
          ? (i < mob->x ? tileObject::RIGHT : tileObject::LEFT)
          : j != mob->y
            ? (j < mob->y ? tileObject::DOWN : tileObject::UP)
            : tileObject::DOWN;

        it = e->mapController.moveMob(it, {e->zLevel, i, j}, direction);
        continue;
      }
      ++it;
    }
  };
  std::thread p (
    [this](std::function<void(std::tuple<int, int, int, int>)> f)
    {
      e->controller<controller::CameraController>()->iterateOverTilesInView(f);
    }, processor
  );
  std::vector<std::pair<std::shared_ptr<MobObject>, std::tuple<int, int>>> movers;
  auto terrainRenderer = [this,&movers](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto terrainObject = e->mapController.terrainMap[e->zLevel].find({ i, j });
    if (terrainObject != e->mapController.terrainMap[e->zLevel].end())
      engine::graphics::controller<engine::graphics::RenderController>.renderCopyTerrain(&terrainObject->second, x, y);
    else
      engine::graphics::controller<engine::graphics::RenderController>.renderCopySprite("Sprite 0x128", x, y);
    auto worldObject = e->mapController.worldMap[e->zLevel].find({ i, j });
    if (worldObject != e->mapController.worldMap[e->zLevel].end())
      for ( auto w : worldObject->second )
        e->gfxController.renderCopyObject(w, x, y);
    auto mobObject = e->mapController.mobMap[e->zLevel].find({ i, j });
    if (mobObject != e->mapController.mobMap[e->zLevel].end())
      for ( auto &w : mobObject->second )
      {
        if (w->relativeX == 0 && w->relativeY == 0)
          engine::graphics::controller<engine::graphics::RenderController>.renderCopyTerrain(&terrainObject->second, x, y);
        else
          movers.push_back({w, { x, y }});
      }
  };
  std::thread r (
    [this, &movers](std::function<void(std::tuple<int, int, int, int>)> f1)
    {
      engine::controller<controller::CameraController>.iterateOverTilesInView(f1);
      auto it = movers.begin();
      while (it != movers.end())
      {
        auto mob = it->first;
        auto [_x, _y] = it->second;
        engine::graphics::controller<engine::graphics::RenderController>.renderCopyMobObject(mob, _x, _y);
        movers.erase(it);
      }
    },
    terrainRenderer
  );
  p.join();
  r.join();
}


int RenderController::renderCopyPlayer()
{
  e->player.x = e->gfxController.camera.x;
  e->player.y = e->gfxController.camera.y;
  auto [_w, _h] = e->gfxController.getWindowGridDimensions();
  SDL_Rect playerRect = { _w/2*e->tileSize, _h/2*e->tileSize, e->tileSize, e->tileSize };
  return SDL_RenderFillRect(e->appRenderer, &playerRect);
}