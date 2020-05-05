#include "graphics.h"
#include "graphics/util.h"
#include "graphics/render.h"
#include "graphics/surface.h"

using namespace graphics;

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