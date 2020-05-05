#ifndef GAME_ENGINE_CAMERA_H
#define GAME_ENGINE_CAMERA_H

#include "engine.h"

struct camera::CameraController
{
  GameEngine* e;
  CameraController () {}
  CameraController (GameEngine* e)
  {
    this->e = e;
  }
  void scrollCamera(int);
  void iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)>);
};

#endif