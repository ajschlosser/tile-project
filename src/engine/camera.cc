#include "engine/graphics.h"
#include "engine/movement.h"
#include "engine/render.h"
#include "engine/camera.h"

using namespace controller;

void CameraController::scrollCamera(int directions)
{
  int x = engine::controller<controller::GraphicsController>.camera.x;
  int y = engine::controller<controller::GraphicsController>.camera.y;
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
      engine::controller<controller::MovementController>.scrollGameSurface(directions);
    else
      SDL_Delay(15);
    engine::controller<controller::GraphicsController>.camera.x = x;
    engine::controller<controller::GraphicsController>.camera.y = y;
  }
  else
  {
    e->controller<controller::RenderController>()->renderCopyTiles();
    e->controller<controller::RenderController>()->renderCopyPlayer();
    e->controller<controller::GraphicsController>()->applyUi();
    e->controller<controller::RenderController>()->renderUI();
    SDL_RenderPresent(e->appRenderer);    
  }
}

void CameraController::iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)> f)
{
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
  int x = 0;
  int y = 0;
  for (auto i = engine::controller<controller::GraphicsController>.camera.x - _w/2; i < engine::controller<controller::GraphicsController>.camera.x + _w/2 + 5; i++)
  {
    for (auto j = engine::controller<controller::GraphicsController>.camera.y - _h/2; j < engine::controller<controller::GraphicsController>.camera.y + _h/2 + 5; j++) {
      std::tuple<int, int, int, int> locationData = {x, y, i, j};
      f(locationData);
      y++;
    }
    y = 0;
    x++;
  }
}