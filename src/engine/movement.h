#ifndef GAME_ENGINE_MOVEMENT_H
#define GAME_ENGINE_MOVEMENT_H

#include "../engine.h"

void GameEngine::processMap(int directions)
{
  auto [_w, _h] = gfxController.getWindowGridDimensions();
  SDL_Point checkCoordinates = { gfxController.camera.x, gfxController.camera.y };
  Rect chunkRect = {
    gfxController.camera.x-configController.gameSize*2,
    gfxController.camera.y-configController.gameSize*2,
    gfxController.camera.x+configController.gameSize*2,
    gfxController.camera.y+configController.gameSize*2
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
  auto it = mapController.terrainMap[zLevel].find({ checkCoordinates.x, checkCoordinates.y });
  if (it == mapController.terrainMap[zLevel].end())
  {
    SDL_Log("Detected ungenerated map: %d %d", checkCoordinates.x, checkCoordinates.y);
    mapController.generateMapChunk(&chunkRect);
  }

};

void GameEngine::scrollGameSurface(int directions)
{
  auto [_w, _h] = gfxController.getWindowDimensions();
  SDL_Rect dest {0, 0, _w, _h};
  std::pair<int, int> offset = {0, 0};
  if (directions & input::RIGHT)
    offset.first -= tileSize;
  if (directions & input::LEFT)
    offset.first += tileSize;
  if (directions & input::UP)
    offset.second += tileSize;
  if (directions & input::DOWN)
    offset.second -= tileSize;
  while (dest.x != offset.first || dest.y != offset.second)
  {
    renderCopyTiles();
    if (directions & input::RIGHT)
      dest.x -= movementSpeed;
    if (directions & input::LEFT)
      dest.x += movementSpeed;
    if (directions & input::UP)
      dest.y += movementSpeed;
    if (directions & input::DOWN)
      dest.y -= movementSpeed;
    SDL_RenderCopy(appRenderer, gfxController.getGameSurfaceTexture(), NULL, &dest);
    renderCopyPlayer();
    gfxController.applyUi();;
    SDL_RenderPresent(appRenderer);
  }
  if (SDL_SetRenderTarget(appRenderer, NULL) < 0)
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not reset render target: %s", SDL_GetError());
}

#endif