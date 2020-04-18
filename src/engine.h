#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "json/json.h"
#include "objects.h"
#include "input.h"

#include <cmath>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <fstream>
#include <memory>

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

struct GameEngine
{
  UserInputHandler userInputHandler;
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
  int zDepth;
  bool generatingChunk;
  std::map<int, BiomeType> biomeTypes;
  std::map<std::string, TileType> tileTypes;
  std::map<std::string, ObjectType> objectTypes;
  std::map<int, std::map<std::pair<int, int>, Tile>> tileMap;
  std::map<int, std::map<std::pair<int, int>, std::shared_ptr<Tile>>> terrainMap;
  std::map<int, std::map<int, std::map<std::pair<int, int>, std::shared_ptr<WorldObject>>>> objectMap;
  std::map<std::string, Sprite> sprites;
  Camera camera;
  GameEngine() : generatingChunk(false), spriteSize(32), running(true), paused(false), refreshed(false), zLevel(0), movementSpeed(8), gameSize(50), zDepth(4) {}
  int init();
  std::pair<int, int> getWindowGridSize();
  int renderCopySprite(std::string spriteName, int x, int y)
  {
    Sprite *s = &sprites[spriteName];
    SDL_Rect src {s->tileMapX, s->tileMapY, spriteSize, spriteSize};
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
  template <class T>
  int renderCopySpriteFrom(std::shared_ptr<T> t, int x, int y)
  {
    Sprite *s = t->tileType->sprite; // gotta genericize tiles more; current tiles should be children of a generic tile object and called something else, i.e. Terrain
    SDL_Rect src {s->tileMapX, s->tileMapY, spriteSize, spriteSize};
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
  template<typename F>
  void iterateOverChunk(SDL_Rect* chunkRect, F f) //std::function<void(int, int, int)>& const lambda)
  {
    int levels = zDepth;
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
      levels, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
    );
    for (auto i = chunkRect->x; i != chunkRect->w; i++)
    {
      for (auto j = chunkRect->y; j != chunkRect->h; j++)
      {
        for (auto h = 0; h < levels; h++)
        {
          f(h, i, j);
        }
      }
    }
  }
  std::shared_ptr<std::map<int, std::map<std::string, int>>> getTilesInRange (SDL_Rect* rangeRect)
  {

    std::map<int, std::map<std::string, int>> tilesInRange;
    auto lambda = [this, &tilesInRange](int h, int i, int j)
    {
      std::shared_ptr<Tile> tileAtCoordinates = terrainMap[h][{i, j}];
      if (tileAtCoordinates)
      {
        std::shared_ptr<Tile> tileAtCoordinates = terrainMap[h][{i, j}];
        tilesInRange[h][tileAtCoordinates->tileType->name] += 1;
      }
    };
    iterateOverChunk(rangeRect, lambda);

    return std::make_shared<std::map<int, std::map<std::string, int>>>(tilesInRange);

  }
  int generateMapChunk(SDL_Rect* chunkRect)
  {
    if (generatingChunk)
    {
      return -1;
    }
    generatingChunk = true;
    BiomeType *b = &biomeTypes[std::rand() % 2];
    while (b->maxDepth <= zLevel || b->maxDepth >= zLevel) b = &biomeTypes[std::rand() % 2];
    SDL_Log("Generating chunk: %s", b->name.c_str());
    auto lambda = [this, b](int h, int i, int j)
    {
      auto tileAtCoordinates = &terrainMap[h][{i, j}];
      if (!tileAtCoordinates->get())
      {
        int index = std::rand() % b->terrainTypes.size();
        Tile newTile = { i, j, "Sprite 0x0", &tileTypes[b->terrainTypes[0].first]};

        std::shared_ptr<Tile> nT = std::make_shared<Tile>();
        nT->x = i;
        nT->y = j;
        nT->type = "Sprite 0x0";
        nT->tileType =  &tileTypes[b->terrainTypes[0].first];
        terrainMap[h][{i, j}] = nT;
        tileMap[h][{i, j}] = newTile;
        int threshold = 1500;
        SDL_Rect rangeRect { i-1, j-1, i+1, j+1 };
        auto tilesInRange = getTilesInRange(&rangeRect); // TODO: only if there are objects
        int layer = 0;
        for (auto relatedObjectType : newTile.tileType->objects)
        {
          if ((*tilesInRange)[h][relatedObjectType])
          {
            threshold = std::pow(threshold, (*tilesInRange)[h][relatedObjectType] + 1);
          }
          if (std::rand() % threshold > 825)
          {
            //WorldObject o2 = { i, j, "Sprite 0x192", &tileTypes[relatedObjectType] };
            //SDL_Log("placing %s", relatedObjectType.c_str());

            std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
              i, j, &tileTypes[relatedObjectType]
            );

            objectMap[h][layer][{o->x, o->y}] = o;
            layer++;
          }
        }
      }
    };
    iterateOverChunk(chunkRect, lambda);
    generatingChunk = false;
    SDL_Log("Created chunk. Map now has %lu tiles", tileMap[0].size());
    return 0;
  }
  void processMap(int directions)
  {

    SDL_Point checkCoordinates = { camera.x, camera.y };
    SDL_Rect chunkRect = { camera.x-3, camera.y-3, camera.x+3, camera.y+3 };

    if (directions & RIGHT)
    {
      checkCoordinates.x += gameSize;

      chunkRect.x += gameSize/2;
      chunkRect.y -= gameSize/2;
      chunkRect.w += gameSize*1.5;
      chunkRect.h += gameSize/2;
    }
    if (directions & LEFT)
    {
      checkCoordinates.x -= gameSize;

      chunkRect.x -= gameSize*1.5;
      chunkRect.y -= gameSize/2;
      chunkRect.w -= gameSize/2;
      chunkRect.h += gameSize/2;
    }
    if (directions & UP)
    {
      checkCoordinates.y -= gameSize;

      chunkRect.x -= gameSize/2;
      chunkRect.y -= gameSize*1.5;
      chunkRect.w += gameSize/2;
      chunkRect.h -= gameSize/2;
    }
    if (directions & DOWN)
    {
      checkCoordinates.y += gameSize;

      chunkRect.x -= gameSize/2;
      chunkRect.y += gameSize/2;
      chunkRect.w += gameSize/2;
      chunkRect.h += gameSize*1.5;
    }

    auto checkTile = &terrainMap[zLevel][{checkCoordinates.x, checkCoordinates.y}];
    // SDL_Log("here: %s", checkTile->get()->tileType->name.c_str());
    if (!checkTile->get())
    {
      SDL_Log("here: %d %d", checkCoordinates.x, checkCoordinates.y);
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
        std::vector<std::shared_ptr<WorldObject>> objects;
        try
        {
          std::shared_ptr<Tile> t = terrainMap[zLevel][{i, j}];
          int layer = 3;
          while (layer >= 0)
          {
            std::shared_ptr<WorldObject> o = objectMap[zLevel][layer][{i, j}];
            if (o) {
              objects.push_back(o);
            }
            layer--;
          }
          if (t)
          {
            renderCopySpriteFrom<Tile>(t, x, y);
          }
          else
          {
            renderCopySprite("Sprite 0x128", x, y);
            SDL_Rect fillChunkRect = {i - gameSize, j - gameSize, i + gameSize, j + gameSize};
            // std::thread foo([this](SDL_Rect* r) {generateMapChunk(r); }, &fillChunkRect);
            // foo.detach();
            // //std::thread th1([this](int d) { processMap(d); }, directions);
            generateMapChunk(&fillChunkRect);
          }
          
        }

        // Handle potential null pointers
        catch (std::exception &e)
        {
          //Tile invalid = {i, j, "Sprite 0x128", &tileTypes["shadow"]};
          //renderCopySprite<Tile>(invalid, x, y);
        }
        for (std::shared_ptr<WorldObject> o : objects)
        {
          renderCopySpriteFrom<WorldObject>(o, x, y);
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
  void scrollCamera(int);
  void handleEvents();
  int renderCopyPlayer();
  int run();
};