#ifndef GAME_ENGINE_GRAPHICS_H
#define GAME_ENGINE_GRAPHICS_H

#include "engine/graphics/render.h"
#include "engine/graphics/surface.h"
#include "engine/graphics/window.h"
#include "engine.h"
#include "objects.h"


struct controller::GraphicsController
{
  GameEngine *e;
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  SDL_Texture* tilemapTexture;
  Image* tilemapImage;
  SDL_DisplayMode displayMode;
  SDL_Rect camera;
  TTF_Font* font;
  std::map<std::string, Sprite> sprites;
  int windowWidth;
  int windowHeight;
  template <typename T> void registerController (T t)
  {
    engine::graphics::controller<T> = t;
  };
  template <typename T> T* controller() { return &engine::graphics::controller<T>; }
  GraphicsController () {}
  GraphicsController (GameEngine* e);
  void applyUI();
};

#endif