#ifndef GAME_ENGINE_EVENTS_H
#define GAME_ENGINE_EVENTS_H

#include "../engine.h"

void GameEngine::handleEvents()
{
  auto keyboardMovementHandler = [this](int directions)
  {
    std::thread graphicalThread([this](int d) { scrollCamera(d); }, directions);
    std::thread mapProcessingThread([this](int d) { processMap(d); }, directions);
    mapProcessingThread.detach();
    graphicalThread.join();
  };
  userInputHandler.handleKeyboardMovement(keyboardMovementHandler);

  auto eventHandler = [this](SDL_Event* event)
  {
    if (event->type == SDL_QUIT)
    {
      SDL_Delay(1500);
      running = false;
    }
    else if (event->type == SDL_KEYDOWN)
    {
      auto t = &mapController.terrainMap[zLevel][{gfxController.camera.x, gfxController.camera.y}];
      std::string objs;
      switch(event->key.keysym.sym)
      {
        case SDLK_ESCAPE:
          SDL_Delay(1500);
          running = false;
          break;
        case SDLK_SPACE:
          SDL_Log(
            "\nCamera: %dx%dx%dx%d\nCurrent terrain type: %s\nCurrent biome type: %s\nCurrent terrain type sprite name: %s\nInitialized: %d",
            gfxController.camera.x,
            gfxController.camera.y,
            gfxController.camera.w,
            gfxController.camera.h,
            t->terrainType->name.c_str(),
            t->biomeType->name.c_str(),
            t->terrainType->getFrame(0)->name.c_str(),
            t->initialized
          );
          break;
        case SDLK_w:
          if (zLevel > 0)
          {
            zLevel--;
            SDL_Log("You are at level %d", zLevel);
          }
          break;
        case SDLK_q:
          if (std::abs(zLevel) < static_cast <int>(mapController.terrainMap.size()))
          {
            if (zLevel < zMaxLevel - 1)
            {
              zLevel++;
            }
            SDL_Log("You are at level %d", zLevel);
          }
          break;
        case SDLK_p:
          tileSize = tileSize / 2;
          configController.tileSize = tileSize;
          break;
        case SDLK_o:
          tileSize = tileSize * 2;
          configController.tileSize = tileSize;
          break;
      }
    }
  };
  userInputHandler.handleAppEvents(eventHandler);
}

#endif