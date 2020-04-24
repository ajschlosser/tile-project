#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include "SDL2/SDL.h"
#include <functional>

enum directions
{
  UP        = 0x01,
  DOWN      = 0x02,
  LEFT      = 0x04,
  RIGHT     = 0x08
};

struct UserInputHandler
{
  SDL_Event appEvent;
  void handleKeyboardMovement (std::function<void(int)>);
  void handleAppEvents (std::function<void(SDL_Event*)>);
};

#endif