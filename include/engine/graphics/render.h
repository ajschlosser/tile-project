#ifndef GAME_ENGINE_GRAPHICS_RENDER_H
#define GAME_ENGINE_GRAPHICS_RENDER_H

#include "engine.h"

struct engine::graphics::RenderController
{
  GameEngine* e;
  RenderController () {}
  RenderController (GameEngine* e)
  {
    this->e = e;
  }
  int renderCopySprite(Sprite*, std::tuple<int, int, int, int>);
  int renderCopySprite(Sprite*, int, int);
  int renderCopySprite(std::string, int, int);
  int renderCopyObject(std::shared_ptr<WorldObject>, int, int);
  int renderCopyMobObject(std::shared_ptr<MobObject>, int, int);
  int renderCopyTerrain(TerrainObject*, int, int);
};

#endif