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
  int renderCopyObject(std::shared_ptr<WorldObject> t, int x, int y)
  {

    if (!t->isAnimated())
      return renderCopySprite(t->objectType->sprite, x, y);
    else
    {
      if (t->animationTimer.elapsed() > t->animationSpeed)
      {
        t->animationTimer.stop();
        t->animationTimer.start();
        t->animationFrame++;
        if (t->animationFrame >= t->objectType->maxFrames())
          t->animationFrame = 0;
      }

      auto it = t->objectType->animationMap.find(t->animationFrame);
      if (it == t->objectType->animationMap.end())
        return renderCopySprite(t->objectType->sprite, x, y);
      else
        return renderCopySprite(it->second, x, y);
    }
  }
  int renderCopyMobObject(std::shared_ptr<MobObject> t, int x, int y)
  {
    if (!t->isAnimated())
      return renderCopySprite(t->mobType->sprite, x, y);
    else
    {
      if (t->animationTimer.elapsed() > t->animationSpeed)
      {
        t->animationTimer.stop();
        t->animationTimer.start();
        t->animationFrame++;
        if (t->animationFrame >= t->mobType->maxFrames())
          t->animationFrame = 0;
      }

      auto it = t->mobType->animationMap.find(t->animationFrame);
      if (it == t->mobType->animationMap.end())
        return renderCopySprite(t->mobType->sprite, x, y);
      else
        return renderCopySprite(it->second, x, y);
    }
  }
  int renderCopyTerrain(TerrainObject* t, int x, int y) {
    if (!t->isAnimated())
      return renderCopySprite(t->terrainType->sprite, x, y);
    else
    {
      if (t->animationTimer.elapsed() > t->animationSpeed)
      {
        t->animationTimer.stop();
        t->animationTimer.start();
        t->animationFrame++;
        if (t->animationFrame >= t->terrainType->maxFrames())
          t->animationFrame = 0;
      }

      auto it = t->terrainType->animationMap.find(t->animationFrame);
      if (it == t->terrainType->animationMap.end())
        return renderCopySprite(t->terrainType->sprite, x, y);
      else
        return renderCopySprite(it->second, x, y);
    }
  }
  SDL_Surface* getGameSurfaceFromWindow();
  SDL_Texture* getTextureFromSurface (SDL_Surface* s);
  SDL_Texture* getGameSurfaceTexture ();
  GraphicsController () {}
};

#endif