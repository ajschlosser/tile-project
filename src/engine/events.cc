#include "engine/camera.h"
#include "engine/events.h"

using namespace controller;

void EventsController::handleEvents()
{
  auto keyboardMovementHandler = [this](int directions)
  {
    std::thread graphicalThread([this](int d) { engine::controller<controller::CameraController>.scrollCamera(d); }, directions);
    std::thread mapProcessingThread([this](int d) { e->processMap(d); }, directions);
    mapProcessingThread.detach();
    graphicalThread.join();
  };
  e->userInputHandler.handleKeyboardMovement(keyboardMovementHandler);

  auto eventHandler = [this](SDL_Event* event)
  {
    if (event->type == SDL_QUIT)
    {
      SDL_Delay(1500);
      e->stopRunning();
    }
    else if (event->type == SDL_KEYDOWN)
    {
      auto t = &e->mapController.terrainMap[e->zLevel][{e->gfxController.camera.x, e->gfxController.camera.y}];
      std::string objs;
      switch(event->key.keysym.sym)
      {
        case SDLK_ESCAPE:
          SDL_Delay(1500);
          e->stopRunning();
          break;
        case SDLK_SPACE:
          SDL_Log(
            "\nCamera: %dx%dx%dx%d\nCurrent terrain type: %s\nCurrent biome type: %s\nCurrent terrain type sprite name: %s\nInitialized: %d",
            e->gfxController.camera.x,
            e->gfxController.camera.y,
            e->gfxController.camera.w,
            e->gfxController.camera.h,
            t->terrainType->name.c_str(),
            t->biomeType->name.c_str(),
            t->terrainType->getFrame(0)->name.c_str(),
            t->initialized
          );
          break;
        case SDLK_w:
          if (e->zLevel > 0)
          {
            e->zLevel--;
            SDL_Log("You are at level %d", e->zLevel);
          }
          break;
        case SDLK_q:
          if (std::abs(e->zLevel) < static_cast <int>(e->mapController.terrainMap.size()))
          {
            if (e->zLevel < e->zMaxLevel - 1)
            {
              e->zLevel++;
            }
            SDL_Log("You are at level %d", e->zLevel);
          }
          break;
        case SDLK_p:
          e->tileSize = e->tileSize / 2;
          e->configController.tileSize = e->tileSize;
          break;
        case SDLK_o:
          e->tileSize = e->tileSize * 2;
          e->configController.tileSize = e->tileSize;
          break;
      }
    }
  };
  e->userInputHandler.handleAppEvents(eventHandler);
}