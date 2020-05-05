#ifndef GAME_GRAPHICS_UTIL_H
#define GAME_GRAPHICS_UTIL_H

#include "graphics.h"

using namespace graphics;

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

#endif