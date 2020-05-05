#include "engine.h"
#include "engine/init.h"
#include "engine/camera.h"
#include "engine/movement.h"
#include "engine/events.h"
#include "engine/render.h"

int GameEngine::run ()
{
  init();
  while (running)
  {
    controller<controller::EventsController>()->handleEvents();
    SDL_RenderClear(appRenderer);
    renderCopyTiles();
    renderCopyPlayer();
    gfxController.applyUi();
    SDL_RenderPresent(appRenderer);
  }
  return 1;
}