#ifndef GAME_ENGINE_CAMERA_H
#define GAME_ENGINE_CAMERA_H

#include "../engine.h"

void GameEngine::scrollCamera(int directions)
{
  int x = gfxController.camera.x;
  int y = gfxController.camera.y;
  if (directions & input::LEFT)
    x--;
  if (directions & input::RIGHT)
    x++;
  if (directions & input::DOWN)
    y++;
  if (directions & input::UP)
    y--;
  if (mapController.isPassable({zLevel, x, y}))
  {
    if (tileSize > 8)
      scrollGameSurface(directions);
    else
      SDL_Delay(15);
    gfxController.camera.x = x;
    gfxController.camera.y = y;
  }
}

void GameEngine::iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)> f)
{
  auto [_w, _h] = gfxController.getWindowGridDimensions();
  int x = 0;
  int y = 0;
  for (auto i = gfxController.camera.x - _w/2; i < gfxController.camera.x + _w/2 + 5; i++)
  {
    for (auto j = gfxController.camera.y - _h/2; j < gfxController.camera.y + _h/2 + 5; j++) {
      std::tuple<int, int, int, int> locationData = {x, y, i, j};
      f(locationData);
      y++;
    }
    y = 0;
    x++;
  }
}

#endif