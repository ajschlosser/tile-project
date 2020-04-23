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

  auto windowSize = getWindowGridDimensions();
  SDL_Log("Current window grid is %dx%d tiles.",
    windowSize.first,
    windowSize.second
  );
  int d = (*tileSize);
  camera = { 0, 0, windowSize.first/d, windowSize.first/d };
  SDL_Log("Camera created at (%d, %d) with %dx%d tile dimensions.",
    0,
    0,
    displayMode.w/d,
    displayMode.h/d
  );

  return 0;
}

std::pair<int, int> GraphicsController::getWindowGridDimensions()
{
  int d = (*tileSize);
  SDL_GetWindowSize(appWindow, &windowWidth, &windowHeight);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Got window size: %dx%d", windowWidth, windowHeight
  );
  int width = static_cast <int> (std::floor(windowWidth/d));
  int height = static_cast <int> (std::floor(windowHeight/d));
  return { width, height };
}

std::tuple<int, int> GraphicsController::getWindowDimensions()
{
  auto p = getWindowGridDimensions();
  return std::make_tuple(p.first, p.second);
}


int GraphicsController::renderCopySprite(Sprite *s, int x, int y)
{
  int tS = (*tileSize);
  SDL_Rect src {s->tileMapX, s->tileMapY, (*spriteSize), (*spriteSize)};
  SDL_Rect dest {x*tS, y*tS, tS, tS};
  if (SDL_RenderCopy(appRenderer, tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Couldn't copy sprite to renderer: %s",
      SDL_GetError()
    );
    return 3;
  }
  return 0;
}

int GraphicsController::renderCopySprite(std::string spriteName, int x, int y)
{
  Sprite *s = &sprites[spriteName];
  return renderCopySprite(s, x, y);
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