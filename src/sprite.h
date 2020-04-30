#ifndef GAME_SPRITE_H
#define GAME_SPRITE_H

#include "SDL2/SDL.h"
#include <string>

struct Image
{
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite
{
  int tileMapX;
  int tileMapY;
  std::string name;
};

#endif