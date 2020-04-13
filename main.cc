#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <cmath>
#include <ctime>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>

enum direction {
  LEFT    = 0x01,
  RIGHT   = 0x02,
  UP      = 0x04,
  DOWN    = 0x08
};

struct Image {
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite {
  int tileMapX;
  int tileMapY;
  std::string tileName;
};

struct WorldObject {
  int x;
  int y;
  std::string type;
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
  int movementSpeed;
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  SDL_Surface* gameSurface;
  SDL_Texture* gameTexture;
  Image tilemapImage;
  SDL_Event appEvent;
  SDL_DisplayMode displayMode;
  int tileSize;
  const int spriteSize;
  int zLevel;
  std::array<std::array<std::array<Tile, 4>, 200>, 200> map;
  std::map<int, std::map<std::pair<int, int>, std::map<std::pair<int, int>, Tile*>>> grid;
  std::map<std::string, Sprite> sprites;
  std::array<std::map<std::pair<int, int>, WorldObject>, 4> objects;
  Camera camera;
  GameEngine() : spriteSize(64), running(true), paused(false), refreshed(false), zLevel(0), movementSpeed(4) {}
  int init()
  {
    std::srand(std::time(nullptr));
    if (!tileSize)
    {
      tileSize = spriteSize;
    }

    // Initialize SDL
    SDL_Log("Initializing SDL libraries...");
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not initialize SDL: %s",
        SDL_GetError()
      );
      return 3;
    }
    else SDL_Log("SDL initialized.");
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not initialize SDL_image: %s",
        IMG_GetError()
      );
      return 3;
    }
    else SDL_Log("SDL_image initialized.");

    // Get current display mode information
    SDL_GetCurrentDisplayMode(0, &displayMode);
    SDL_Log("Current display is %dx%dpx.",
      displayMode.w,
      displayMode.h
    );

    // Create camera
    auto windowSize = getWindowSize();
    SDL_Log("Current window is %dx%dpx.",
      windowSize.first,
      windowSize.second
    );
    camera = { 15, 15, windowSize.first/tileSize, windowSize.first/tileSize };
    SDL_Log("Camera created with %dx%d tile dimensions.",
      displayMode.w/tileSize,
      displayMode.h/tileSize
    );

    // Create app window and renderer
    appWindow = SDL_CreateWindow("tile-project", 0, 0, displayMode.w/2, displayMode.h/2, SDL_WINDOW_RESIZABLE);
    if (appWindow == NULL)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not create app window: %s",
        SDL_GetError()
      );
      return 3;
    }
    else
    {
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
        "Window created."
      );
    }
    appRenderer = SDL_CreateRenderer(appWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (appRenderer == NULL)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Couldn't create a enderer: %s",
        SDL_GetError()
      );
      return 3;
    }
    else
    {
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
        "Renderer created."
      );
    }

    // Load spritesheet
    SDL_Surface *surface = IMG_Load("tilemap.png");
    if (!surface)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "IMG_Load error: %s", IMG_GetError());
      return 3;
    }
    else
    {
      SDL_Log("Loaded spritesheet is %dx%dpx sheet of %dx%dpx tiles.",
        surface->w,
        surface->h,
        spriteSize,
        spriteSize
      );
    }

    // Create sprites from spritesheet
    SDL_Texture *texture = SDL_CreateTextureFromSurface(appRenderer, surface);
    if (!texture) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Couldn't create texture from surface: %s",
        SDL_GetError()
      );
      return 3;
    }
    else
    {
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
        "Tilemap loaded."
      );
    }
    tilemapImage = { surface, texture };
    for (auto i = 0; i < surface->w; i += spriteSize) {
      for (auto j = 0; j < surface->h; j += spriteSize) {
        std::string name = "Sprite " + std::to_string(i) + "x" + std::to_string(j);
        Sprite s { i, j, name };
        sprites[name] = s;
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
          "Created tile: %s",
          name.c_str()
        );
      }
    }
    SDL_Log("Spritesheet processed.");

    // Create default tilemap
    for (auto i = 0; i < map.size(); i++) {
      for (auto j = 0; j < map.size(); j++) {
        Tile top { i, j, "Sprite 64x0" };
        Tile middle { i, j, "Sprite 64x64" };
        Tile bottom { i, j, "Sprite 64x128" };
        int n = std::rand() % 150;
        if (n > 80)
        {
          top.type = "Sprite 64x64";
        }
        if (n > 98)
        {
          top.type = "Sprite 64x128";
          middle.type = "Sprite 64x128";
        }
        if (n > 99)
        {
          bottom.type = "Sprite 64x64";
        }
        map.at(i).at(j).at(0) = top;
        map.at(i).at(j).at(1) = middle;
        map.at(i).at(j).at(2) = bottom;
        map.at(i).at(j).at(3) = Tile {i,j,"Sprite 64x64"};
      }
    }
    objects.at(0)[{ 0, 0 }] = WorldObject {15, 15, "Sprite 64x256"};
    SDL_Log("Tilemap of %lu tiles created.",
      map.size()*map.size()*map.at(0).at(0).size()
    );
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
  template <class T>
  int renderCopySprite(T* t, int x, int y) {
    Sprite s = sprites[t->type];
    SDL_Rect src {s.tileMapX, s.tileMapY, spriteSize, spriteSize};
    SDL_Rect dest {x*tileSize, y*tileSize, tileSize, tileSize};
    SDL_RenderCopy(appRenderer, tilemapImage.texture, &src, &dest);
    return 0;
  }
  void animate(int directions)
  {
    auto gridSize = getWindowSize();
    int _w = gridSize.first*tileSize;
    int _h = gridSize.second*tileSize;
    SDL_Log("Current window is %dx%dpx.", _w, _h);
    gameSurface = SDL_CreateRGBSurface(0, _w, _h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_Rect dest {0, 0, _w, _h};
    std::pair<int, int> offset = {0, 0};
    if (directions & RIGHT)
    {
      offset.first -= tileSize;
    }
    if (directions & LEFT)
    {
      offset.first += tileSize;
    }
    if (directions & UP)
    {
      offset.second += tileSize;
    }
    if (directions & DOWN)
    {
      offset.second -= tileSize;
    }
    SDL_Log("offset: %d %d \t dest: %d %d", offset.first, offset.second, dest.x, dest.y);
    while (dest.x != offset.first || dest.y != offset.second)
    {
      SDL_Log("animating: offset: %d %d \t dest: %d %d", offset.first, offset.second, dest.x, dest.y);
      SDL_RenderClear(appRenderer);
      renderCopyTiles();
      SDL_RenderReadPixels(appRenderer, NULL, SDL_PIXELFORMAT_UNKNOWN, gameSurface->pixels, gameSurface->pitch);
      gameTexture = SDL_CreateTextureFromSurface(appRenderer, gameSurface);
      if (directions & RIGHT) {
        dest.x -= movementSpeed;
      }
      if (directions & LEFT) {
        dest.x += movementSpeed;
      }
      if (directions & UP) {
        dest.y += movementSpeed;
      }
      if (directions & DOWN) {
        dest.y -= movementSpeed;
      }
      SDL_Rect src {tileSize, tileSize, _w-tileSize, _h-tileSize};
      SDL_RenderCopy(appRenderer, gameTexture, NULL, &dest);
      SDL_RenderPresent(appRenderer);
    }
    if (SDL_SetRenderTarget(appRenderer, NULL) < 0) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not reset render target: %s",
        SDL_GetError()
      );
    }
  }
  void renderCopyTiles()
  {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "renderCopyTiles() called"
    );
    auto windowSize = getWindowSize();
    int x = 0;
    int y = 0;
    for (auto i = camera.x - windowSize.first/2 - 3; i < camera.x + windowSize.first/2 + 3; i++) {
      for (auto j = camera.y - windowSize.second/2; j < camera.y + windowSize.second/2 + 3; j++) {
        Tile t;
        try
        {
          t = map.at(i).at(j).at(zLevel);
          
        }
        catch (std::exception &e)
        {
          t = {i, j, "Sprite 64x128"};
        }
        renderCopySprite<Tile>(&t, x, y);
        y++;
      }
      y = 0;
      x++;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "renderCopyTiles() completed. Screen refreshed."
    );
  }
  void scrollCamera(int directions) {
    animate(directions);
    if (directions & LEFT) {
      camera.x -= 1;
    }
    if (directions & RIGHT) {
      camera.x += 1;
    }
    if (directions & DOWN) {
      camera.y += 1;
    }
    if (directions & UP) {
      camera.y -= 1;
    }
  }
  void handleEvents()
  {
    SDL_PumpEvents();
    auto *ks = SDL_GetKeyboardState(NULL);
    while(ks[SDL_SCANCODE_LEFT]
        || ks[SDL_SCANCODE_RIGHT]
        || ks[SDL_SCANCODE_UP]
        || ks[SDL_SCANCODE_DOWN]
      )
    {
      if ((ks[SDL_SCANCODE_DOWN] && ks[SDL_SCANCODE_UP])
          || (ks[SDL_SCANCODE_LEFT] && ks[SDL_SCANCODE_RIGHT])
        )
      {
        break;
      }

      int directions = 0x00;
      if (ks[SDL_SCANCODE_LEFT])
      {
        directions += LEFT;
      }
      if (ks[SDL_SCANCODE_RIGHT])
      {
        directions += RIGHT;
      }
      if (ks[SDL_SCANCODE_UP])
      {
        directions += UP;
      }
      if (ks[SDL_SCANCODE_DOWN])
      {
        directions += DOWN;
      }
      scrollCamera(directions);
      SDL_PumpEvents();
    }
    SDL_PollEvent(&appEvent);
    if (appEvent.type == SDL_QUIT) {
      running = false;
    }
    else if (appEvent.type == SDL_KEYDOWN) {
      switch(appEvent.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_SPACE:
          SDL_Log("Camera: %dx%dx%dx%d",
            camera.x,
            camera.y,
            camera.w,
            camera.h
          );
          break;
        case SDLK_w:
          if (zLevel > 0)
          {
            zLevel--;
          }
          break;
        case SDLK_q:
          SDL_Log("You are at level %d", zLevel);
          if (std::abs(zLevel) < static_cast <int>(map.at(0).at(0).size())) {
            zLevel++;
          }
          break;
        case SDLK_p:
          paused = !paused;
          break;
      }
    }
    else if (appEvent.type == SDL_MOUSEBUTTONDOWN)
    {
      if (appEvent.button.button == SDL_BUTTON_LEFT) {

      }
    }
    else if (appEvent.type == SDL_WINDOWEVENT)
    {
      switch (appEvent.window.event)
      {
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
      SDL_RenderClear(appRenderer);
      renderCopyTiles();
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
