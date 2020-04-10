#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <cmath>
#include <ctime>
#include <array>
#include <map>
#include <string>
#include <utility>

struct Image {
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite {
  int tileMapX;
  int tileMapY;
  std::string tileName;
};

struct Tile {
  int x;
  int y;
  std::string type;
};

struct Camera {
  int x;
  int y;
  int w;
  int h;
};

struct GameEngine {
  bool running;
  bool paused;
  bool refreshed;
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  Image tilemapImage;
  SDL_Event appEvent;
  SDL_DisplayMode displayMode;
  int tileSize;
  const int spriteSize;
  std::array<std::array<Tile, 500>, 500> map;
  std::map<std::string, Sprite> sprites;
  Camera camera;
  GameEngine() : spriteSize(64), running(true), paused(false), refreshed(false) {}
  int init()
  {
    std::srand(std::time(nullptr));
    if (!tileSize) {
      tileSize = spriteSize;
    }
    SDL_Log("Initializing SDL libraries...");
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL: %s", SDL_GetError());
      return 3;
    } else SDL_Log("SDL initialized.");
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL_image: %s", IMG_GetError());
      return 3;
    } else SDL_Log("SDL_image initialized.");
    SDL_Surface *surface = IMG_Load("tilemap.png");
    if (!surface) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMG_Load error: %s", IMG_GetError());
      return 3;
    } else SDL_Log("Loaded spritesheet is %dx%dpx sheet of %dx%dpx tiles.", surface->w, surface->h, spriteSize, spriteSize);
    SDL_GetCurrentDisplayMode(0, &displayMode);
    SDL_Log("Current display is %dx%dpx.", displayMode.w, displayMode.h);
    auto windowSize = getWindowSize();
    SDL_Log("Current window is %dx%dpx.", windowSize.first, windowSize.second);
    camera = { 15, 15, windowSize.first/tileSize, windowSize.first/tileSize };
    SDL_Log("Camera created with %dx%d tile dimensions.", displayMode.w/tileSize, displayMode.h/tileSize);
    if (SDL_CreateWindowAndRenderer(displayMode.w, displayMode.h, SDL_WINDOW_RESIZABLE, &appWindow, &appRenderer)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
      return 3;
    } else SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Window created.");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(appRenderer, surface);
    if (!texture) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
      return 3;
    } else SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Tilemap loaded.");
    tilemapImage = {surface, texture};
    for (auto i = 0; i < surface->w; i += spriteSize) {
      for (auto j = 0; j < surface->h; j += spriteSize) {
        std::string name = "Tile " + std::to_string(i) + "x" + std::to_string(j);
        Sprite s{i,j,name};
        sprites[name] = s;
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Created tile: %s", name.c_str());
      }
    }
    SDL_Log("Spritesheet processed.");
    for (auto i = 0; i < map.size(); i++) {
      for (auto j = 0; j < map.size(); j++) {
        Tile t {i, j, "Tile 64x0"};
        int n = std::rand() % 100;
        if (n > 80) {
          t.type = "Tile 64x64";
        }
        if (n > 98) {
          t.type = "Tile 64x128";
        }
        map.at(i).at(j) = t;
      }
    }
    SDL_Log("Tilemap of %lu tiles created.", map.size());
    return 0;
  }
  int quit()
  {
    return 0;
  }
  std::pair<int, int> getWindowSize()
  {
    int _w, _h;
    SDL_GetWindowSize(appWindow, &_w, &_h);
    int width = static_cast <int> (std::floor(_w/tileSize));
    int height = static_cast <int> (std::floor(_h/tileSize));
    return {width, height};
  }
  int renderCopyImage(Image* i, int x, int y)
  {
    if (!i->texture || !i->surface) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderCopyImage() failed: texture or surface was null");
      return 3;
    }
    SDL_Rect dest {x, y, i->surface->w, i->surface->h};
    SDL_RenderCopy(appRenderer, i->texture, NULL, &dest);
    return 0;
  }
  int renderCopyTile(Tile* t, int x, int y) {
    Sprite s = sprites[t->type];
    SDL_Rect src {s.tileMapX, s.tileMapY, spriteSize, spriteSize};
    SDL_Rect dest {x*tileSize, y*tileSize, tileSize, tileSize};
    SDL_RenderCopy(appRenderer, tilemapImage.texture, &src, &dest);
    return 0;
  }
  void renderClearAndPresent()
  {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "renderClearAndPresent() called");
    SDL_SetRenderDrawColor(appRenderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(appRenderer);
    auto windowSize = getWindowSize();
    int x = 0;
    int y = 0;
    for (auto i = camera.x - windowSize.first/2; i < camera.x + windowSize.first/2; i++) {
      for (auto j = camera.y - windowSize.second/2; j < camera.y + windowSize.second/2; j++) {
        Tile t;
        try
        {
          t = map.at(i).at(j);
        }
        catch (std::exception &e) {
          t = {i, j, "Tile 0x0"};
        }
        if (x == std::round(windowSize.first/2) && y == std::round(windowSize.second/2)) {
          t = {i, j, "Tile 64x256"};
        }
        renderCopyTile(&t, x, y);
        y++;
      }
      y = 0;
      x++;
    }
    SDL_RenderPresent(appRenderer);
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "renderClearAndPresent() completed. Screen refreshed.");
    refreshed = true;
  }
  void handleEvents()
  {
    SDL_PollEvent(&appEvent);
    if (appEvent.type == SDL_QUIT) {
      running = false;
    }
    else if (appEvent.type == SDL_KEYDOWN) {
      refreshed = false;
      switch(appEvent.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_LEFT:
          if (camera.x > 0) camera.x = camera.x - 1;
          break;
        case SDLK_RIGHT:
          camera.x = camera.x + 1;
          break;
        case SDLK_DOWN:
          if (camera.y > 0) camera.y = camera.y - 1;
          break;
        case SDLK_UP:
          camera.y = camera.y + 1;
          break;
        case SDLK_SPACE:
          SDL_Log("Camera: %dx%dx%dx%d",
            camera.x,
            camera.y,
            camera.w,
            camera.h
          );
          break;
        case SDLK_p:
          paused = !paused;
          break;
      }
    }
    else if (appEvent.type == SDL_WINDOWEVENT) {
      switch (appEvent.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
          SDL_Log("Window %d size changed to %dx%d",
            appEvent.window.windowID,
            appEvent.window.data1,
            appEvent.window.data2
          );
      }
    }
  }
  int run()
  {
    while (running) {
      handleEvents();
      if (!paused && !refreshed) renderClearAndPresent();
    }
    return 1;
  }
};

int main() {
  GameEngine engine;
  engine.init();
  engine.run();
  return 0;
}
