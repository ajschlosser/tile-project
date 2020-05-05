#ifndef GAME_ENGINE_MOVEMENT_H
#define GAME_ENGINE_MOVEMENT_H

#include "engine.h"

struct controller::MovementController
{
  GameEngine* e;
  MovementController () {}
  MovementController (GameEngine* e)
  {
    this->e = e;
  }
  void processMap(int);
  void scrollGameSurface (int);
};

#endif