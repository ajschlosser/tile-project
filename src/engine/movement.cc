
#include "engine/graphics.h"
#include "engine/render.h"
#include "engine/movement.h"

using namespace controller;

void MovementController::processMap(int directions)
{
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
  SDL_Point checkCoordinates = { engine::controller<controller::GraphicsController>.camera.x, engine::controller<controller::GraphicsController>.camera.y };
  Rect chunkRect = {
    engine::controller<controller::GraphicsController>.camera.x-e->configController.gameSize*2,
    engine::controller<controller::GraphicsController>.camera.y-e->configController.gameSize*2,
    engine::controller<controller::GraphicsController>.camera.x+e->configController.gameSize*2,
    engine::controller<controller::GraphicsController>.camera.y+e->configController.gameSize*2
  };
  if (directions & input::RIGHT)
    checkCoordinates.x += _w;
    chunkRect.x1 += _w/2;
  if (directions & input::LEFT)
    checkCoordinates.x -= _w;
    chunkRect.x1 -= _w/2;
  if (directions & input::UP)
    checkCoordinates.y -= _h;
    chunkRect.y1 += _h/2;
  if (directions & input::DOWN)
    checkCoordinates.y += _h;
    chunkRect.y1 -= _h/2;
  auto it = e->mapController.terrainMap[e->zLevel].find({ checkCoordinates.x, checkCoordinates.y });
  if (it == e->mapController.terrainMap[e->zLevel].end())
  {
    SDL_Log("Detected ungenerated map: %d %d", checkCoordinates.x, checkCoordinates.y);
    e->mapController.generateMapChunk(&chunkRect);
  }

};

void MovementController::scrollGameSurface(int directions)
{
  auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowDimensions();
  SDL_Rect dest {0, 0, _w, _h};
  std::pair<int, int> offset = {0, 0};
  if (directions & input::RIGHT)
    offset.first -= e->tileSize;
  if (directions & input::LEFT)
    offset.first += e->tileSize;
  if (directions & input::UP)
    offset.second += e->tileSize;
  if (directions & input::DOWN)
    offset.second -= e->tileSize;
  while (dest.x != offset.first || dest.y != offset.second)
  {
    e->controller<controller::RenderController>()->renderCopyTiles();
    if (directions & input::RIGHT)
      dest.x -= e->movementSpeed;
    if (directions & input::LEFT)
      dest.x += e->movementSpeed;
    if (directions & input::UP)
      dest.y += e->movementSpeed;
    if (directions & input::DOWN)
      dest.y -= e->movementSpeed;
    SDL_RenderCopy(e->appRenderer, engine::graphics::controller<engine::graphics::SurfaceController>.getGameSurfaceTexture(), NULL, &dest);
    e->controller<controller::RenderController>()->renderCopyPlayer();
    engine::controller<controller::GraphicsController>.applyUi();;
    SDL_RenderPresent(e->appRenderer);
  }
  if (SDL_SetRenderTarget(e->appRenderer, NULL) < 0)
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not reset render target: %s", SDL_GetError());
}