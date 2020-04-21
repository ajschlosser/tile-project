#include "input.h"

void UserInputHandler::handleKeyboardMovement (std::function<void(int)> f)
{
  auto *ks = SDL_GetKeyboardState(NULL);
  while(ks[SDL_SCANCODE_LEFT]
      || ks[SDL_SCANCODE_RIGHT]
      || ks[SDL_SCANCODE_UP]
      || ks[SDL_SCANCODE_DOWN]
    )
  {
    if ((ks[SDL_SCANCODE_DOWN] && ks[SDL_SCANCODE_UP])
        || (ks[SDL_SCANCODE_LEFT] && ks[SDL_SCANCODE_RIGHT])
      )
    {
      break;
    }
    int directions = 0x00;
    if (ks[SDL_SCANCODE_LEFT])
    {
      directions += LEFT;
    }
    if (ks[SDL_SCANCODE_RIGHT])
    {
      directions += RIGHT;
    }
    if (ks[SDL_SCANCODE_UP])
    {
      directions += UP;
    }
    if (ks[SDL_SCANCODE_DOWN])
    {
      directions += DOWN;
    }
    SDL_Log("moving");
    f(directions);
    SDL_PumpEvents();
  }
};