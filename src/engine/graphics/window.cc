#include "engine/graphics.h"
#include "engine/graphics/window.h"

using namespace engine::graphics;

std::tuple<int, int> WindowController::getWindowGridDimensions()
{
  auto gfx = e->controller<controller::GraphicsController>();
  SDL_GetWindowSize(e->appWindow, &gfx->windowWidth, &gfx->windowHeight);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Got window size: %dx%d", gfx->windowWidth, gfx->windowHeight
  );
  int _w = static_cast <int> (std::floor(gfx->windowWidth/e->getTileSize()));
  int _h = static_cast <int> (std::floor(gfx->windowHeight/e->getTileSize()));
  return std::make_tuple(_w, _h);
}

std::tuple<int, int> WindowController::getWindowDimensions()
{
  SDL_Rect viewportRect;
  SDL_RenderGetViewport(e->appRenderer, &viewportRect);
  int _w = viewportRect.w;
  int _h = viewportRect.h;
  return std::make_tuple(_w, _h);
}