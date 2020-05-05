#ifndef GAME_ENGINE_RENDER_H
#define GAME_ENGINE_RENDER_H

#include "engine.h"

struct controller::RenderController
{
  GameEngine* e;
  RenderController () {}
  RenderController (GameEngine* e)
  {
    this->e = e;
  }
  void renderCopyTiles();
  int renderCopyPlayer();
};

#endif