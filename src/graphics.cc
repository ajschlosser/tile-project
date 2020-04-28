#include "graphics.h"

int GraphicsController::initializeSDL ()
{
  SDL_Log("Initializing SDL libraries...");
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Could not initialize SDL: %s",
      SDL_GetError()
    );
    return 3;
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
    return 3;
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

  auto [_w, _h] = getWindowGridDimensions();
  SDL_Log("Current window grid is %dx%d tiles.", _w, _h);
  camera = { 0, 0, _w/(*tileSize), _h/(*tileSize) };
  SDL_Log("Camera created at (%d, %d) with %dx%d tile dimensions.", 0, 0,
    displayMode.w/(*tileSize),
    displayMode.h/(*tileSize)
  );
  return 0;
}

std::tuple<int, int>GraphicsController::getWindowGridDimensions()
{
  SDL_GetWindowSize(appWindow, &windowWidth, &windowHeight);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Got window size: %dx%d", windowWidth, windowHeight
  );
  int _w = static_cast <int> (std::floor(windowWidth/(*tileSize)));
  int _h = static_cast <int> (std::floor(windowHeight/(*tileSize)));
  return std::make_tuple(_w, _h);
}

std::tuple<int, int> GraphicsController::getWindowDimensions()
{
  SDL_Rect viewportRect;
  SDL_RenderGetViewport(appRenderer, &viewportRect);
  int _w = viewportRect.w;
  int _h = viewportRect.h;
  return std::make_tuple(_w, _h);
}


int GraphicsController::renderCopySprite(Sprite *s, int x, int y)
{
  int tS = (*tileSize);
  SDL_Rect src {s->tileMapX, s->tileMapY, (*spriteSize), (*spriteSize)};
  SDL_Rect dest {x*tS, y*tS, tS, tS};
  if (SDL_RenderCopy(appRenderer, tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int GraphicsController::renderCopySprite(std::string spriteName, int x, int y)
{
  auto it = sprites.find(spriteName);
  if (it != sprites.end())
    return renderCopySprite(&it->second, x, y);
  else
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not render sprite: %s", spriteName.c_str());
    return -1;
}

SDL_Surface* GraphicsController::getGameSurfaceFromWindow ()
{
  #if SDL_BYTEORDER == SDL_BIG_ENDIAN
      auto rmask = 0xff000000;
      auto gmask = 0x00ff0000;
      auto bmask = 0x0000ff00;
      auto amask = 0x000000ff;
  #else
      auto rmask = 0x000000ff;
      auto gmask = 0x0000ff00;
      auto bmask = 0x00ff0000;
      auto amask = 0xff000000;
  #endif
  auto [_w, _h] = getWindowDimensions();
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Current window is %dx%dpx.", _w, _h );
  SDL_Surface* s = SDL_CreateRGBSurface(0, _w, _h, 32, rmask, gmask, bmask, amask);
  if (s == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create RGB surface: %s", SDL_GetError());
    return NULL;
  }
  return s;
}

SDL_Texture* GraphicsController::getTextureFromSurface (SDL_Surface* s)
{
  if (SDL_MUSTLOCK(s))
  {
    if (SDL_LockSurface(s) < 0)
      return NULL;
    if (SDL_RenderReadPixels(appRenderer, NULL, SDL_PIXELFORMAT_RGBA32, s->pixels, s->pitch) < 0)
      return NULL;
    SDL_UnlockSurface(s);
  }
  else
  {
    if (SDL_RenderReadPixels(appRenderer, NULL, SDL_PIXELFORMAT_RGBA32, s->pixels, s->pitch) < 0)
      return NULL;
  }
  SDL_Texture* t = SDL_CreateTextureFromSurface(appRenderer, s);
  return t;
}

SDL_Texture* GraphicsController::getGameSurfaceTexture ()
{
  SDL_Surface* s = getGameSurfaceFromWindow();
  if (s)
    return getTextureFromSurface(s);
  else
    return NULL;
}

void GraphicsController::applyUi()
{
  int tS = (*tileSize);
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