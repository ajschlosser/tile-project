#ifndef GAME_ENGINE_GRAPHICS_SURFACE_H
#define GAME_ENGINE_GRAPHICS_SURFACE_H

#include "engine.h"

struct engine::graphics::SurfaceController
{
  GameEngine* e;
  SurfaceController () {}
  SurfaceController (GameEngine* e)
  {
    this->e = e;
  }
  SDL_Surface* getGameSurfaceFromWindow ();
  SDL_Texture* getTextureFromSurface (SDL_Surface*);
  SDL_Texture* getGameSurfaceTexture ();
};

#endif