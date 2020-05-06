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
  GraphicsController (GameEngine* e)
  {
    this->e = e;
    auto renderController = engine::graphics::RenderController(e);
    auto surfaceControlller = engine::graphics::SurfaceController(e);
    auto windowController = engine::graphics::WindowController(e);
    engine::graphics::controller<engine::graphics::RenderController> = renderController;
    engine::graphics::controller<engine::graphics::SurfaceController> = surfaceControlller;
    engine::graphics::controller<engine::graphics::WindowController> = windowController;

    SDL_Log("Initializing SDL libraries...");
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not initialize SDL: %s",
        SDL_GetError()
      );
    }
    else
    {
      SDL_Log("SDL initialized.");
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not initialize SDL_image: %s",
        IMG_GetError()
      );
    }
    else
    {
      SDL_Log("SDL_image initialized.");
    }
    SDL_GetCurrentDisplayMode(0, &displayMode);
    SDL_Log("Current display is %dx%dpx.",
      displayMode.w,
      displayMode.h
    );
    if(TTF_Init()==-1) {
      SDL_Log("TTF_Init: %s\n", TTF_GetError());
    }
    else
    {
      SDL_Log("SDL_ttf initialized.");
    }
    auto [_w, _h] = engine::graphics::controller<engine::graphics::WindowController>.getWindowGridDimensions();
    SDL_Log("Current window grid is %dx%d tiles.", _w, _h);
    camera = { 0, 0, _w/e->getTileSize(), _h/e->getTileSize() };
    SDL_Log("Camera created at (%d, %d) with %dx%d tile dimensions.", 0, 0,
      displayMode.w/e->getTileSize(),
      displayMode.h/e->getTileSize()
    );

  }
  void applyUi()
  {
    int tS = e->getTileSize();
    SDL_Rect viewportRect;
    SDL_RenderGetViewport(appRenderer, &viewportRect);
    SDL_Rect leftRect = {0, 0, tS, viewportRect.h};
    SDL_Rect rightRect = {viewportRect.w-tS, 0, tS, viewportRect.h};
    SDL_Rect topRect = {0, 0, viewportRect.w, tS};
    SDL_Rect bottomRect = {0, viewportRect.h-tS*2, viewportRect.w, tS*2};
    SDL_RenderFillRect(appRenderer, &leftRect);
    SDL_RenderFillRect(appRenderer, &rightRect);
    SDL_RenderFillRect(appRenderer, &topRect);
    SDL_RenderFillRect(appRenderer, &bottomRect);
  }
};

#endif