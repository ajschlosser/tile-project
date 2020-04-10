#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <array>
#include <string>
#include <vector>

struct Image {
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Tile {
  int tileMapX;
  int tileMapY;
  std::string tileName;
};

struct T {
  int x;
  int y;
  std::string type;
  SDL_Rect rect;
};

struct GameEngine {
  bool running;
  SDL_Window* appWindow;
  SDL_Surface* gameSurface;
  SDL_Surface* uiSurface;
  SDL_Renderer* appRenderer;
  Image tilemapImage;
  SDL_Event appEvent;
  SDL_DisplayMode displayMode;
  const int tileSize;
  std::vector<Tile> tiles;
  std::array<std::array<T*, 100>, 100> map;
  GameEngine() : tileSize(64), running(true) {}
  int init()
  {
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
    } else SDL_Log("Loaded spritesheet is %dx%dpx sheet of %dx%dpx tiles.", surface->w, surface->h, tileSize, tileSize);
    SDL_GetCurrentDisplayMode(0, &displayMode);
    SDL_Log("Current display is %dx%dpx.", displayMode.w, displayMode.h);
    if (SDL_CreateWindowAndRenderer(displayMode.w/2, displayMode.h/2, SDL_WINDOW_RESIZABLE, &appWindow, &appRenderer)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
      return 3;
    } else SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Window created.");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(appRenderer, surface);
    if (!texture) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
      return 3;
    } else SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Tilemap loaded.");
    tilemapImage = {surface, texture};
    for (auto i = 0; i < surface->w; i += tileSize) {
      for (auto j = 0; j < surface->h; j += tileSize) {
        std::string name = "Tile " + std::to_string(i) + "x" + std::to_string(j);
        Tile t{i,j,name};
        tiles.push_back(t);
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Created tile: %s", name.c_str());
      }
    }
    for (auto i = 0; i < map.size(); i += tileSize) {
      for (auto j = 0; j < map.size(); j += tileSize) {
        T *t;
        createTileFromBrush(t, "Tile 64x64", i, j);
        map.at(i).at(j) = t;
      }
    }
    return 0;
  }
  void createTileFromBrush(T *t, std::string brushName, int x, int y)
  {
    t->x = x;
    t->y = y;
    t->type = "Unknown";
    for (auto tile : tiles) {
      if (tile.tileName == brushName) {
        SDL_Rect r {tile.tileMapX, tile.tileMapY, tileSize, tileSize};
        t->rect = r;
        t->type = tile.tileName;
      }
    }
  }
  int quit()
  {
    return 0;
  }
  int renderCopyImage(SDL_Renderer* renderer, Image* i, int x, int y)
  {
    if (!i->texture || !i->surface) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "renderCopyImage() failed: texture or surface was null");
      return 3;
    }
    SDL_Rect dest {x, y, i->surface->w, i->surface->h};
    SDL_RenderCopy(renderer, i->texture, NULL, &dest);
    return 0;
  }
  int renderCopyTile(T* t) {
    SDL_Rect dest {t->x, t->y, tileSize, tileSize};

    SDL_RenderCopy(appRenderer, tilemapImage.texture, &t->rect, &dest);
    return 0;
  }
  int run()
  {
    running = true;
    while (running) {
      SDL_PollEvent(&appEvent);
      if (appEvent.type == SDL_QUIT) {
        running = false;
      }
      else if (appEvent.type == SDL_KEYDOWN) {
        switch(appEvent.key.keysym.sym) {
          case SDLK_ESCAPE:
            running = false;
            break;
        }
      }
      SDL_SetRenderDrawColor(appRenderer, 0x00, 0x00, 0x00, 0x00);
      SDL_RenderClear(appRenderer);
      renderCopyImage(appRenderer, &tilemapImage, 300, 300);
      T *tt = map.at(10).at(10);
      SDL_Log("%s", tt->type.c_str());
      renderCopyTile(tt);
      SDL_RenderPresent(appRenderer);
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
