#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <string>
#include <vector>
#include <iostream>

SDL_Window* window;
SDL_Surface* surface;
SDL_Surface* ui;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;
SDL_DisplayMode dm;

const int kTileSize = 64;

struct Tile {
  int tileMapX;
  int tileMapY;
  std::string tileName;
};


std::vector<Tile> tiles;

int main() {
  SDL_Log("Initializing SDL libraries...");
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL: %s", SDL_GetError());
    return 3;
  } else SDL_Log("SDL initialized.");
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL_image: %s", IMG_GetError());
    return 3;
  } else SDL_Log("SDL_image initialized.");
  SDL_GetCurrentDisplayMode(0, &dm);
  SDL_Log("Current display is %d by %d pixels.", dm.w, dm.h);

  SDL_Surface *tilemap = IMG_Load("tilemap.png");
  if (!tilemap) {
    SDL_Log("IMG_Load error: %s", IMG_GetError());
    return -1;
  }
  SDL_Log("Loaded tilemap is %dx%d pixel map of %dx%d pixel tiles.", tilemap->w, tilemap->h, kTileSize, kTileSize);
  for (auto i = 0; i < tilemap->w; i += kTileSize) {
    for (auto j = 0; j < tilemap->h; j += kTileSize) {
      std::string name = "Tile " + std::to_string(i) + "x" + std::to_string(j);
      Tile t{i,j,name};
      tiles.push_back(t);
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Created tile: %s", name.c_str());
    }
  }

  if (SDL_CreateWindowAndRenderer(dm.w/2, dm.h/2, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    return 3;
  }

  texture = SDL_CreateTextureFromSurface(renderer, tilemap);
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
    return 3;
  }

  ui = SDL_CreateRGBSurface(0, dm.w, dm.h, 16, 0, 0, 0, 255);
  SDL_FillRect(ui, NULL, SDL_MapRGB(ui->format, 0, 0, 255));
  SDL_Texture *uit = SDL_CreateTextureFromSurface(renderer, ui);

  SDL_FreeSurface(ui);
  SDL_FreeSurface(surface);
  SDL_FreeSurface(tilemap);

  bool running = true;
  while (running) {
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) {
      running = false;
    }
    else if (event.type == SDL_KEYDOWN) {
      switch(event.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;
      }
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_Rect dest{0,0,100,100};
    SDL_RenderCopy(renderer, uit, NULL, &dest);
    //Tile t = tiles.at(12);
    //SDL_Rect r_src{t.tileMapX, t.tileMapY, kTileSize, kTileSize};
    //SDL_Rect r_dst{0, 0, 64, 64};
    //SDL_RenderCopy(renderer, texture, &r_src, &r_dst);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  IMG_Quit();
  SDL_Quit();
  return 0;
}
