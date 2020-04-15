#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "json/json.h"
#include <cmath>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <fstream>

enum direction 
{
  LEFT    = 0x01,
  RIGHT   = 0x02,
  UP      = 0x04,
  DOWN    = 0x08
};

struct Image
{
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite
{
  int tileMapX;
  int tileMapY;
  std::string tileName;
};

struct GenericType
{
  Sprite* sprite;
  std::string name;
};

struct TileType;
struct ObjectType : GenericType
{
  std::vector<TileType*> tiles;
};

struct TileType : GenericType
{
  std::vector<ObjectType*> objects;
};

struct BiomeType : GenericType
{
  std::vector<TileType*> tiles;
};

struct Tile
{
  int x;
  int y;
  std::string type;
  TileType* tileType;
  bool exists ()
  {
    return type.length() > 0;
  }
};

struct WorldObject : Tile {};

struct Player {
  int x;
  int y;
  std::string type;
  int hp;
};

struct Camera
{
  int x;
  int y;
  int w;
  int h;
};

struct GameEngine
{
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
  Player player;
  int tileSize;
  int gameSize;
  const int spriteSize;
  int zLevel;
  std::map<std::string, TileType> tileTypes;
  std::map<int, std::map<std::pair<int, int>, Tile>> tileMap;
  std::map<int, std::map<int, std::map<std::pair<int, int>, WorldObject>>> objectMap;
  std::map<std::string, Sprite> sprites;
  Camera camera;
  GameEngine() : spriteSize(32), running(true), paused(false), refreshed(false), zLevel(0), movementSpeed(8), gameSize(100) {}
  int init()
  {
    player = {0, 0, "Sprite 0x96", 100};
    int n = 500;
    while (n > 0)
    {
      WorldObject o = {std::rand() % gameSize, std::rand() % gameSize, "Sprite 0x192"};
      objectMap[0][0][{o.x, o.y}] = o;
      n--;
    }
    n = 1000;
    while (n > 0)
    {
      WorldObject o = {std::rand() % gameSize, std::rand() % gameSize, "Sprite 0x224"};
      objectMap[0][2][{o.x, o.y}] = o;
      n--;
    }
    n = 500;
    while (n > 0)
    {
      WorldObject o = {std::rand() % gameSize, std::rand() % gameSize, "Sprite 32x192"};
      objectMap[0][3][{o.x, o.y}] = o;
      n--;
    }
    n = 500;
    while (n > 0)
    {
      WorldObject o = {std::rand() % gameSize, std::rand() % gameSize, "Sprite 0x256"};
      objectMap[1][4][{o.x, o.y}] = o;
      n--;
    }
    std::srand(std::time(nullptr));
    if (!tileSize)
    {
      tileSize = spriteSize;
    }
    if (movementSpeed > tileSize)
    {
      movementSpeed = tileSize;
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
    auto windowSize = getWindowGridSize();
    SDL_Log("Current window is %dx%dpx.",
      windowSize.first,
      windowSize.second
    );
    camera = { gameSize/2, gameSize/2, windowSize.first/tileSize, windowSize.first/tileSize };
    SDL_Log("Camera created at (%d, %d) with %dx%d tile dimensions.",
      gameSize/2,
      gameSize/2,
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
    SDL_Surface *surface = IMG_Load("assets/tilemap640x640x32.png");
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
    if (!texture)
    {
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
    for (auto i = 0; i < surface->w; i += spriteSize)
    {
      for (auto j = 0; j < surface->h; j += spriteSize)
      {
        std::string name = "Sprite " + std::to_string(i) + "x" + std::to_string(j);
        Sprite s { i, j, name };
        sprites[name] = s;
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
          "Created sprite: %s",
          name.c_str()
        );
      }
    }
    SDL_Log("Spritesheet processed.");

    SDL_Log("Reading tilemap configuration file and creating tiles from sprites.");
    std::ifstream tileConfigJsonFile("tilemap.config.json");

    Json::Value tileConfigJson;
    tileConfigJsonFile >> tileConfigJson;

    for (auto i = 0; i < tileConfigJson["tiles"].size(); ++i)
    {
      std::string spriteName = tileConfigJson["tiles"][i]["sprite"].asString();
      std::string tileTypeName = tileConfigJson["tiles"][i]["name"].asString();
      TileType t { &sprites[spriteName], tileTypeName };
      tileTypes[tileTypeName] = t;
    }

    SDL_Log("Generating default tilemap...");
    // Create default tilemap
    for (auto i = 0; i < gameSize*2; i++)
    {
      for (auto j = 0; j < gameSize*2; j++)
      {
        Tile top { i, j, "Sprite 0x0", &tileTypes["grass"] };
        Tile middle { i, j, "Sprite 0x32", &tileTypes["soil"] };
        Tile bottom { i, j, "Sprite 0x64", &tileTypes["rock"] };
        int n = std::rand() % 150;
        if (n > 80)
        {
          top.type = "Sprite 0x32";
          top.tileType = &tileTypes["soil"];
        }
        if (n > 98)
        {
          top.type = "Sprite 0x64";
          middle.type = "Sprite 0x64";
          top.tileType = &tileTypes["rock"];
          middle.tileType = &tileTypes["rock"];
        }
        if (n > 99)
        {
          bottom.type = "Sprite 0x32";
        }
        tileMap[0][{i, j}] = top;
        tileMap[1][{i, j}] = middle;
        tileMap[2][{i, j}] = bottom;
        tileMap[3][{i, j}] = Tile {i,j,"Sprite 0x64", &tileTypes["rock"]};
      }
    }
    SDL_Log("Tilemap of %d tiles created.",
      gameSize*gameSize*4
    );
    return 0;
  }
  std::pair<int, int> getWindowGridSize()
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
    if (SDL_RenderCopy(appRenderer, tilemapImage.texture, &src, &dest) < -1)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Couldn't copy sprite to renderer: %s",
        SDL_GetError()
      );
      return 3;
    }
    return 0;
  }
  void scrollGameSurface(int directions)
  {
    SDL_Rect viewportRect;
    SDL_RenderGetViewport(appRenderer, &viewportRect);
    int _w = viewportRect.w;
    int _h = viewportRect.h;

    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "Current window is %dx%dpx.",
      _w,
      _h
    );
    Uint32 rmask, gmask, bmask, amask;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif
    gameSurface = SDL_CreateRGBSurface(0, _w, _h, 32, rmask, gmask, bmask, amask);
    if (!gameSurface)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not create RGB surface: %s",
        SDL_GetError()
      );
    }
    
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
    while (dest.x != offset.first || dest.y != offset.second)
    {
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
        "scrollGameSurface: offset: %d %d \t dest: %d %d",
        offset.first,
        offset.second,
        dest.x,
        dest.y
      );
      if (SDL_RenderClear(appRenderer) < 0)
      {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
          "Could not clear renderer: %s",
          SDL_GetError()
        );
        break;
      }
      renderCopyTiles();
      if (SDL_LockSurface(gameSurface) < 0)
      {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
          "Could not lock surface for pixel access: %s",
          SDL_GetError()
        );
        break;
      }
      if (SDL_RenderReadPixels(appRenderer, NULL, SDL_PIXELFORMAT_RGBA32, gameSurface->pixels, gameSurface->pitch) < 0)
      {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
          "Could not read pixels from renderer: %s",
          SDL_GetError()
        );
        break;
      }
      SDL_UnlockSurface(gameSurface);
      gameTexture = SDL_CreateTextureFromSurface(appRenderer, gameSurface);
      if (directions & RIGHT)
      {
        dest.x -= movementSpeed;
      }
      if (directions & LEFT)
      {
        dest.x += movementSpeed;
      }
      if (directions & UP)
      {
        dest.y += movementSpeed;
      }
      if (directions & DOWN)
      {
        dest.y -= movementSpeed;
      }
      SDL_RenderCopy(appRenderer, gameTexture, NULL, &dest);
      renderCopyPlayer();
      applyUi();
      SDL_RenderPresent(appRenderer);
      SDL_DestroyTexture(gameTexture);
    }
    if (SDL_SetRenderTarget(appRenderer, NULL) < 0)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
        "Could not reset render target: %s",
        SDL_GetError()
      );
    }
  }
  void applyUi()
  {
    SDL_Rect viewportRect;
    SDL_RenderGetViewport(appRenderer, &viewportRect);
    SDL_Rect leftRect = {0, 0, tileSize, viewportRect.h};
    SDL_Rect rightRect = {viewportRect.w-tileSize, 0, tileSize, viewportRect.h};
    SDL_Rect topRect = {0, 0, viewportRect.w, tileSize};
    SDL_Rect bottomRect = {0, viewportRect.h-tileSize*2, viewportRect.w, tileSize*2};
    SDL_RenderFillRect(appRenderer, &leftRect);
    SDL_RenderFillRect(appRenderer, &rightRect);
    SDL_RenderFillRect(appRenderer, &topRect);
    SDL_RenderFillRect(appRenderer, &bottomRect);
  }
  template<typename Functor>
  void iterateOverChunk(SDL_Rect* chunkRect, Functor f) //std::function<void(int, int, int)>& const lambda)
  {
    int levels = tileMap.size();
    for (auto h = 0; h < levels; h++)
    {
      SDL_Log("Processing chunk: on level %d from ( %d, %d ) to ( %d, %d )",
        h, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
      );
      for (auto i = chunkRect->x; i != chunkRect->w; i++)
      {
        for (auto j = chunkRect->y; j != chunkRect->h; j++)
        {
          f(h, i, j);
        }
      }
    }          
  }
  void generateMapChunk(SDL_Rect* chunkRect)
  {

    auto lambda = [this](int h, int i, int j)
    {
      Tile *tileAtCoordinates = &tileMap[h][{i, j}];
      if (!tileAtCoordinates->exists())
      {
        Tile newTile = {i, j, "Sprite 0x32"};
        if (std::rand() % 100 > 90) newTile.type = "Sprite 0x96";
        tileMap[h][{i, j}] = newTile;
      }
    };
    iterateOverChunk(chunkRect, lambda);

    SDL_Log("Created chunk. Map now has %lu tiles", tileMap[0].size());
  }
  void processMap(int directions)
  {

    SDL_Point checkCoordinates = { camera.x, camera.y };
    SDL_Rect chunkRect = { camera.x-1, camera.y-1, camera.x+1, camera.y+1 };

    if (directions & RIGHT)
    {
      checkCoordinates.x += gameSize/2;

      chunkRect.x += gameSize/2;
      chunkRect.y -= gameSize/2;
      chunkRect.w += gameSize*1.5;
      chunkRect.h += gameSize/2;
    }
    if (directions & LEFT)
    {
      checkCoordinates.x -= gameSize/2;

      chunkRect.x -= gameSize*1.5;
      chunkRect.y -= gameSize/2;
      chunkRect.w -= gameSize/2;
      chunkRect.h += gameSize/2;
    }
    if (directions & UP)
    {
      checkCoordinates.y -= gameSize/2;

      chunkRect.x -= gameSize/2;
      chunkRect.y -= gameSize*1.5;
      chunkRect.w += gameSize/2;
      chunkRect.h -= gameSize/2;
    }
    if (directions & DOWN)
    {
      checkCoordinates.y += gameSize/2;

      chunkRect.x -= gameSize/2;
      chunkRect.y += gameSize/2;
      chunkRect.w += gameSize/2;
      chunkRect.h += gameSize*1.5;
    }

    Tile *checkTile = &tileMap[zLevel][{checkCoordinates.x, checkCoordinates.y}];

    if (!checkTile->exists())
    {
      generateMapChunk(&chunkRect);
    }
  
  };

  void renderCopyTiles()
  {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "renderCopyTiles() called"
    );
    auto windowSize = getWindowGridSize();
    int x = 0;
    int y = 0;
    for (auto i = camera.x - windowSize.first/2; i < camera.x + windowSize.first/2 + 3; i++)
    {
      for (auto j = camera.y - windowSize.second/2; j < camera.y + windowSize.second/2 + 3; j++)
      {
        Tile* t;
        std::vector<WorldObject*> objects;
        try
        {
          t = &tileMap[zLevel][{i, j}];
          int layer = 3;
          while (layer >= 0)
          {
            WorldObject *o;
            o = &objectMap[zLevel][layer][{i, j}];
            if (o->exists()) {
              objects.push_back(o);
            }
            layer--;
          }
          renderCopySprite<Tile>(t, x, y);
        }

        // Handle potential null pointers
        catch (std::exception &e)
        {
          Tile invalid = {i, j, "Sprite 0x128"};
          renderCopySprite<Tile>(&invalid, x, y);
        }
        for (WorldObject *o : objects)
        {
          renderCopySprite<WorldObject>(o, x, y);
        }
        y++;
      }
      y = 0;
      x++;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "renderCopyTiles() completed. Screen refreshed."
    );
  }
  void scrollCamera(int directions)
  {
    scrollGameSurface(directions);
    if (directions & LEFT)
    {
      camera.x -= 1;
    }
    if (directions & RIGHT)
    {
      camera.x += 1;
    }
    if (directions & DOWN)
    {
      camera.y += 1;
    }
    if (directions & UP)
    {
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
      std::thread th1([this](int d) { processMap(d); }, directions);
      std::thread th2([this](int d) { scrollCamera(d); }, directions);
      th1.detach();
      th2.join();
      SDL_PumpEvents();
    }
    SDL_PollEvent(&appEvent);
    if (appEvent.type == SDL_QUIT)
    {
      running = false;
    }
    else if (appEvent.type == SDL_KEYDOWN)
    {
      std::string tileType = tileMap[zLevel][{camera.x, camera.y}].tileType->name;
      switch(appEvent.key.keysym.sym)
      {
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_SPACE:
          SDL_Log("Camera: %dx%dx%dx%d %s",
            camera.x,
            camera.y,
            camera.w,
            camera.h,
            tileType.c_str()
          );
          break;
        case SDLK_w:
          if (zLevel > 0)
          {
            zLevel--;
            SDL_Log("You are at level %d", zLevel);
          }
          break;
        case SDLK_q:
          if (std::abs(zLevel) < static_cast <int>(tileMap.size()))
          {
            zLevel++;
            SDL_Log("You are at level %d", zLevel);
          }
          break;
        case SDLK_p:
          tileSize = tileSize / 2;
          break;
        case SDLK_o:
          tileSize = tileSize * 2;
          break;
      }
    }
    else if (appEvent.type == SDL_MOUSEBUTTONDOWN)
    {
      if (appEvent.button.button == SDL_BUTTON_LEFT)
      {

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
  int renderCopyPlayer()
  {
    player.x = camera.x;
    player.y = camera.y;
    auto gridSize = getWindowGridSize();
    return renderCopySprite<Player>(&player, gridSize.first/2, gridSize.second/2);
  }
  int run()
  {
    while (running)
    {
      handleEvents();
      SDL_RenderClear(appRenderer);
      renderCopyTiles();
      renderCopyPlayer();
      applyUi();
      SDL_RenderPresent(appRenderer);
    }
    return 1;
  }
};

int main()
{
  GameEngine engine;
  engine.tileSize = 64;
  engine.init();
  engine.run();
  return 0;
}
