#include "SDL2/SDL.h"
#include <functional>

enum directions
{
  UP      = 0x01,
  DOWN    = 0x02,
  LEFT    = 0x04,
  RIGHT   = 0x08
};

struct UserInputHandler
{
  void handleKeyboardEvents (std::function<void(int)> f);
};