#include "engine/camera.h"

using namespace controller;

void CameraController::scrollCamera(int directions)
{
  int x = e->gfxController.camera.x;
  int y = e->gfxController.camera.y;
  if (directions & input::LEFT)
    x--;
  if (directions & input::RIGHT)
    x++;
  if (directions & input::DOWN)
    y++;
  if (directions & input::UP)
    y--;
  if (e->mapController.isPassable({e->zLevel, x, y}))
  {
    if (e->tileSize > 8)
      e->scrollGameSurface(directions);
    else
      SDL_Delay(15);
    e->gfxController.camera.x = x;
    e->gfxController.camera.y = y;
  }
}

void CameraController::iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)> f)
{
  auto [_w, _h] = e->gfxController.getWindowGridDimensions();
  int x = 0;
  int y = 0;
  for (auto i = e->gfxController.camera.x - _w/2; i < e->gfxController.camera.x + _w/2 + 5; i++)
  {
    for (auto j = e->gfxController.camera.y - _h/2; j < e->gfxController.camera.y + _h/2 + 5; j++) {
      std::tuple<int, int, int, int> locationData = {x, y, i, j};
      f(locationData);
      y++;
    }
    y = 0;
    x++;
  }
}