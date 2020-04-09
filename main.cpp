#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <string>
#include <vector>

SDL_Window* window;
SDL_Surface* surface;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;

int kTileSize = 64;

struct Tile {
  int tileMapX;
  int tileMapY;
  std::string tileName;
};


std::vector<Tile> tiles;

int main() {
  SDL_Log("Initializing SDL libraries...");
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
    SDL_Log("Could not initialize SDL.");
  } else SDL_Log("SDL initialized.");
  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    SDL_Log("Could not initialize SDL_image: %s", IMG_GetError()); 
  } else SDL_Log("SDL_image initialized.");

  SDL_Surface *tilemap = IMG_Load("tilemap.png");
  if (!tilemap) {
    SDL_Log("IMG_Load error: %s", IMG_GetError());
    return -1;
  }
  SDL_Log("Loaded tilemap is %dx%d pixel map of %dx%d pixel tiles.", tilemap->w, tilemap->h, kTileSize, kTileSize);
  for (auto i = 0; i < tilemap->w; i += kTileSize) {
    for (auto j = 0; j < tilemap->h; j += kTileSize) {
      std::string name = "Tile " + std::to_string(i) + "x" + std::to_string(j);
      tiles.push_back(Tile{i,j,name});
      SDL_Log("Created tile: %s", name.c_str());
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not intialize SDL: %s", SDL_GetError());
    return 3;
  }
  if (SDL_CreateWindowAndRenderer(1024, 900, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError());
    return 3;
  }

  texture = SDL_CreateTextureFromSurface(renderer, tilemap);
  if (!texture) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
    return 3;
  }

  SDL_FreeSurface(surface);
  SDL_FreeSurface(tilemap);

  while (1) {
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) {
      break;
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    //SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  IMG_Quit();
  SDL_Quit();
  return 0;
}
