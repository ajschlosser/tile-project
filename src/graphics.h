#ifndef GAME_GRAPHICS_H
#define GAME_GRAPHICS_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "objects.h"
#include <map>
#include <string>
#include <memory>

struct GraphicsController
{
  SDL_Renderer* appRenderer;
  SDL_Texture* tilemapTexture;
  Image* tilemapImage;
  SDL_DisplayMode displayMode;
  std::map<std::string, Sprite> sprites;
  int* tileSize;
  int* spriteSize;
  int initializeSDL();
  int renderCopySprite(std::string, int, int);
  void applyUi();
  int renderCopySprite(Sprite*, int, int);
  template<class T> int renderCopySpriteFrom(std::shared_ptr<T> t, int x, int y) { return renderCopySprite(t->tileType->sprite, x, y); }
  GraphicsController () {}
};

#endif