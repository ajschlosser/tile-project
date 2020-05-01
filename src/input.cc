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
      break;
    int directions = 0x00;
    if (ks[SDL_SCANCODE_LEFT])
      directions += input::LEFT;
    if (ks[SDL_SCANCODE_RIGHT])
      directions += input::RIGHT;
    if (ks[SDL_SCANCODE_UP])
      directions += input::UP;
    if (ks[SDL_SCANCODE_DOWN])
      directions += input::DOWN;
    f(directions);
    SDL_PumpEvents();
  }
};

void UserInputHandler::handleAppEvents (std::function<void(SDL_Event*)> f)
{
  SDL_PollEvent(&appEvent);
  f(&appEvent);
};