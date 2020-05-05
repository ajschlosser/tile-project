#ifndef GAME_ENGINE_GRAPHICS_WINDOW_H
#define GAME_ENGINE_GRAPHICS_WINDOW_H

#include "engine.h"

struct engine::graphics::WindowController
{
  GameEngine* e;
  WindowController () {}
  WindowController (GameEngine* e)
  {
    this->e = e;
  }
  std::tuple<int, int> getWindowGridDimensions();
  std::tuple<int, int> getWindowDimensions();
};

#endif