#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include <cmath>
#include <ctime>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>

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

struct Timer {
  int last;
  int current;
  bool paused;
  bool started;
  void start()
  {
    started = true;
    paused = false;
    current = 0;
    last = SDL_GetTicks();
  }
  void stop()
  {
    started = false;
    paused = false;
    last = 0;
    current = 0;
  }
  void pause()
  {
    if (started && !paused)
    {
      paused = true;
      current = SDL_GetTicks() - last;
      last = 0;
    }
  }
  void unpause()
  {
    if (started && paused) {
      paused = false;
      last = SDL_GetTicks() - current;
      current = 0;
    }
  }
  int elapsed()
  {
    auto time = 0;
    if (started)
    {
      if (paused)
      {
        time = current;
      }
      else
      {
        time = SDL_GetTicks() - last;
      }
    }
    return time;
  }
  Timer () : last(0), current(0), paused(false), started(false) {}
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
  Timer fpsTimer;
  int tileSize;
  const int spriteSize;
  int zLevel;
  std::array<std::array<std::array<Tile, 4>, 250>, 250> map;
  std::map<int, std::map<std::pair<int, int>, std::map<std::pair<int, int>, Tile*>>> grid;
  std::map<std::string, Sprite> sprites;
  std::array<std::map<std::pair<int, int>, WorldObject>, 4> objects;
  Camera camera;
  GameEngine() : spriteSize(64), running(true), paused(false), refreshed(false), zLevel(0) {}
  int init()
  {
    fpsTimer.start();
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
    appWindow = SDL_CreateWindow("tile-project", 0, 0, displayMode.w, displayMode.h, SDL_WINDOW_RESIZABLE);
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
    if (SDL_CreateWindowAndRenderer(displayMode.w, displayMode.h, SDL_WINDOW_RESIZABLE, &appWindow, &appRenderer))
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Couldn't create window and renderer: %s",
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
  int renderCopyImage(Image* i, int x, int y)
  {
    if (!i->texture || !i->surface)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "renderCopyImage() failed: texture or surface was null"
      );
      return 3;
    }
    SDL_Rect dest {x, y, i->surface->w, i->surface->h};
    SDL_RenderCopy(appRenderer, i->texture, NULL, &dest);
    return 0;
  }
  template <class T>
  int renderCopySprite(T* t, int x, int y) {
    Sprite s = sprites[t->type];
    SDL_Rect src {s.tileMapX, s.tileMapY, spriteSize, spriteSize};
    SDL_Rect dest {x*tileSize, y*tileSize, tileSize, tileSize};
    SDL_RenderCopy(appRenderer, tilemapImage.texture, &src, &dest);
    return 0;
  }
  void renderCopyTiles()
  {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "renderCopyTiles() called"
    );
    auto windowSize = getWindowSize();
    int x = 0;
    int y = 0;
    for (auto i = camera.x - windowSize.first/2; i < camera.x + windowSize.first/2; i++) {
      for (auto j = camera.y - windowSize.second/2; j < camera.y + windowSize.second/2; j++) {
        Tile t;
        try
        {
          t = map.at(i).at(j).at(zLevel);
          //grid[{x, y}][{i, j}] = &map.at(i).at(j).at(zLevel);
          
        }
        catch (std::exception &e)
        {
          t = {i, j, "Sprite 64x128"};
        }
        // if (x == std::round(windowSize.first/2) && y == std::round(windowSize.second/2))
        // {
        //   t = {i, j, "Sprite 64x256"};
        // }
        renderCopySprite<Tile>(&t, x, y);
        y++;
      }
      y = 0;
      x++;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "renderCopyTiles() completed. Screen refreshed."
    );
    refreshed = true;
  }
  //void renderCopyWorldObject(WorldObject* w)
  void renderUi()
  {
    SDL_SetRenderDrawColor(appRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    auto windowSize = getWindowSize();
    SDL_Rect bottom {
      0,
      windowSize.second*tileSize - tileSize*3,
      static_cast<int>(std::round(windowSize.first*tileSize)),
      static_cast<int>(std::round(windowSize.second*tileSize))
    };
    SDL_RenderFillRect(appRenderer, &bottom);
    bottom.x += 5;
    bottom.y += 5;
    bottom.w -= 5;
    bottom.h -= 5;
    SDL_SetRenderDrawColor(appRenderer, 50, 50, 50, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(appRenderer, &bottom);
  }
  void handleEvents()
  {
    SDL_PollEvent(&appEvent);
    if (appEvent.type == SDL_QUIT) {
      running = false;
    }
    else if (appEvent.type == SDL_KEYDOWN && !appEvent.key.repeat) {
      refreshed = false;
      switch(appEvent.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_LEFT:
          if (camera.x > 0)
          {
            camera.x -= 1;
          }
          break;
        case SDLK_RIGHT:
          if (camera.x < map.size())
          {
            camera.x += 1;
          }
          break;
        case SDLK_UP:
          if (camera.y > 0)
          {
            camera.y -= 1;
          }
          break;
        case SDLK_DOWN:
          if (camera.y < map.size())
          {
            camera.y += + 1;
          }
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
        // int x = appEvent.button.x/tileSize;
        // int y = appEvent.button.y/tileSize;
        // auto gridTile = grid[{x, y}];
        // for (auto const& [coordinates, object] : gridTile)
        // {
        //   SDL_Log("%s at (%d, %d) (grid[{%d, %d}][%d, %d])",
        //     object->type.c_str(),
        //     coordinates.first,
        //     coordinates.second,
        //     x,
        //     y,
        //     coordinates.first,
        //     coordinates.second
        //   );
        // }
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
      if (!refreshed) {
        SDL_SetRenderDrawColor(appRenderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(appRenderer);
        renderCopyTiles();

        for (auto const& [coordinates, object] : objects.at(zLevel))
        {
          auto windowSize = getWindowSize();
          int x = coordinates.first;
          int y = coordinates.second;
          WorldObject o = objects.at(zLevel)[coordinates];
          SDL_Log("There is a '%s' at tile (%d, %d) and at (%d, %d) in the window", o.type.c_str(), x, y, o.x, o.y);
          if (x > camera.x - 15 && x < camera.x + 15 && y > camera.y - 15 && y < camera.y + 15)
          {
            renderCopySprite<WorldObject>(&o, o.x, o.y);
          }
        }

        renderUi();
        refreshed = true;
        SDL_RenderPresent(appRenderer);
      }
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
