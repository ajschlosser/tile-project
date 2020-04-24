#ifndef GAME_GRAPHICS_H
#define GAME_GRAPHICS_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "objects.h"
#include <cmath>
#include <map>
#include <string>
#include <memory>
#include <tuple>

struct GraphicsController
{
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  SDL_Texture* tilemapTexture;
  Image* tilemapImage;
  SDL_DisplayMode displayMode;
  SDL_Rect camera;
  std::map<std::string, Sprite> sprites;
  int windowWidth;
  int windowHeight;
  int* tileSize;
  int* spriteSize;
  int initializeSDL();
  std::tuple<int, int> getWindowGridDimensions();
  std::tuple<int, int> getWindowDimensions();
  void applyUi();
  int renderCopySprite(std::string, int, int);
  int renderCopySprite(Sprite*, int, int);
  template<class T> int renderCopyObject(std::shared_ptr<T> t, int x, int y) { return renderCopySprite(t->sprite, x, y); }
  template<class T> int renderCopySpriteFrom(T* t, int x, int y) { return renderCopySprite(t->terrainType->sprite, x, y); }
  int renderCopyTile(TileObject* t, std::tuple<int, int> coords) { auto [x, y] = coords; return renderCopySprite(t->getTerrainTypeSprite(), x, y); }
  SDL_Surface* getGameSurfaceFromWindow();
  SDL_Texture* getTextureFromSurface (SDL_Surface* s);
  SDL_Texture* getGameSurfaceTexture ();
  GraphicsController () {}
};

#endif