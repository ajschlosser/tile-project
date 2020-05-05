#include "../graphics.h"

using namespace graphics;

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