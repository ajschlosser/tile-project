#include "SDL2/SDL.h"
#include "engine.h"

int GameEngine::init()
{
  player = {gameSize/2, gameSize/2, &tileTypes["water"]};
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

  for (auto i = 0; i < tileConfigJson["terrains"].size(); ++i)
  {
    std::string spriteName = tileConfigJson["terrains"][i]["sprite"].asString();
    std::string tileTypeName = tileConfigJson["terrains"][i]["name"].asString();
    std::vector<std::string> relatedObjectTypes;
    const Json::Value& relatedObjectsArray = tileConfigJson["terrains"][i]["objects"];
    for (int i = 0; i < relatedObjectsArray.size(); i++)
    {
      relatedObjectTypes.push_back(relatedObjectsArray[i].asString());
    }
    SDL_Log("- Loaded '%s' terrain", tileTypeName.c_str());
    TileType t { &sprites[spriteName], tileTypeName, relatedObjectTypes };
    auto t2 = std::make_shared<TerrainType> (&sprites[spriteName], tileTypeName, relatedObjectTypes);
    tileTypes[tileTypeName] = t;
    terrainTypes[tileTypeName] = t2;
  }
  for (auto i = 0; i < tileConfigJson["biomes"].size(); ++i)
  {
    //std::string spriteName = tileConfigJson["objects"][i]["sprite"].asString();
    std::string biomeTypeName = tileConfigJson["biomes"][i]["name"].asString();
    BiomeType b = { biomeTypeName };
    b.maxDepth = tileConfigJson["biomes"][i]["maxDepth"].asInt();
    b.minDepth = tileConfigJson["biomes"][i]["minDepth"].asInt();
    const Json::Value& terrainsArray = tileConfigJson["biomes"][i]["terrains"];
    for (int i = 0; i < terrainsArray.size(); i++)
    {
      auto t = terrainsArray[i];
      b.terrainTypes[b.terrainTypes.size()] = { t["name"].asString(), t["multiplier"].asFloat() };
    }
    biomeTypes[biomeTypes.size()] = b;
    SDL_Log("- Loaded '%s' biome", biomeTypeName.c_str());
  }
  for (auto i = 0; i < tileConfigJson["objects"].size(); ++i)
  {
    std::string spriteName = tileConfigJson["objects"][i]["sprite"].asString();
    std::string objectTypeName = tileConfigJson["objects"][i]["name"].asString();
    const Json::Value& biomesArray = tileConfigJson["objects"][i]["biomes"];

    std::map<std::string, int> bM;
    for (int i = 0; i < biomesArray.size(); i++)
    {
      bM[biomesArray[i].asString()] = 1;
    }

    SDL_Log("- Loaded '%s' object", objectTypeName.c_str());

    TileType t { &sprites[spriteName], objectTypeName };
    tileTypes[objectTypeName] = t;
    ObjectType o { &sprites[spriteName], objectTypeName, bM };
    objectTypes[objectTypeName] = o;
  }

  // Create default tilemap
  SDL_Log("Generating default tilemap...");
  SDL_Rect initialChunk = { 0, 0, gameSize*2, gameSize*2 };
  generateMapChunk(&initialChunk);

  SDL_Log("Tilemap of %d tiles created.",
    gameSize*gameSize*4
  );
  return 0;
}

std::pair<int, int> GameEngine::getWindowGridSize()
{
  int _w, _h;
  SDL_GetWindowSize(appWindow, &_w, &_h);
  int width = static_cast <int> (std::floor(_w/tileSize));
  int height = static_cast <int> (std::floor(_h/tileSize));
  return {width, height};
}

void GameEngine::scrollCamera(int directions)
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


void GameEngine::processMap(int directions)
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

  auto checkTerrainObject = &terrainMap[zLevel][{checkCoordinates.x, checkCoordinates.y}];
  if (!checkTerrainObject->get())
  {
    SDL_Log("here: %d %d", checkCoordinates.x, checkCoordinates.y);
    generateMapChunk(&chunkRect);
  }

};


int GameEngine::generateMapChunk(SDL_Rect* chunkRect)
{
  if (mapGenerator.currentlyGenerating == true)
  {
    return -1;
  }
  mapGenerator.currentlyGenerating = true;
  mapGenerator.currentBiomeType = &biomeTypes[std::rand() % biomeTypes.size()];
  SDL_Log("Generating chunk: %s", mapGenerator.currentBiomeType->name.c_str());

  auto createTerrainPlaceholders = [this](int h, int i, int j)
  {
    auto terrainObjectAtCoordinates = &terrainMap[h][{i, j}];
    while (h > mapGenerator.currentBiomeType->maxDepth || h < mapGenerator.currentBiomeType->minDepth)
    {
      mapGenerator.currentBiomeType = &biomeTypes[std::rand() % biomeTypes.size()];
    }
    auto b = mapGenerator.currentBiomeType;
    if (!terrainObjectAtCoordinates->get())
    {
      std::shared_ptr<TerrainObject> nT = std::make_shared<TerrainObject>();
      nT->x = i;
      nT->y = j;
      nT->biomeType = b;
      terrainMap[h][{i, j}] = nT;

    }
  };

  auto growBiomes = [this](int h, int i, int j)
  {
    auto t = getTerrainObjectAt(h, i, j);
    SDL_Rect r { i-1, j-1, i+1, j+1 };
    auto counts = getCountsInRange(&r);
    if (counts["biome"]["grass"] > 2)
    {
      //SDL_Log("grassy");
    }
    //auto biomesInRange = (*getBiomesInRange(&r));
    // if (biomesInRange[h][t->get()->biomeType->name] < 2)
    // {
    //   SDL_Log("not much");
    // }
  };

  auto setTerrainTypes = [this](int h, int i, int j)
  {
    auto t = getTerrainObjectAt(h, i, j);
    if (t->get())
    {
      t->get()->terrainType = terrainTypes[t->get()->biomeType->terrainTypes[0].first];
      t->get()->tileType = &tileTypes[t->get()->biomeType->terrainTypes[0].first];
    }
  };

  auto addWorldObjects = [this](int h, int i, int j)
  {
    auto terrainObjectAtCoordinates = &terrainMap[h][{i, j}];
    int layer = 0;
    for (auto relatedObjectType : terrainObjectAtCoordinates->get()->tileType->objects)
    {
      int threshold = 1000;
      if (!objectTypes[relatedObjectType].biomes[terrainObjectAtCoordinates->get()->biomeType->name])
      {
        continue;
      }
      if (std::rand() % 1000 > 825)
      {
        std::shared_ptr<WorldObject> o = std::make_shared<WorldObject>(
          i, j, &tileTypes[relatedObjectType]
        );
        objectMap[h][layer][{o->x, o->y}] = o;
        layer++;
      }
    }
  };


  iterateOverChunk(chunkRect, createTerrainPlaceholders);
  randomlyAccessAllTilesInChunk(chunkRect, growBiomes);
  iterateOverChunk(chunkRect, setTerrainTypes);
  iterateOverChunk(chunkRect, addWorldObjects);
  mapGenerator.currentlyGenerating = false;
  SDL_Log("Created chunk. Map now has %lu tiles", terrainMap[0].size());
  return 0;
}


std::shared_ptr<std::map<int, std::map<std::string, int>>> GameEngine::getTilesInRange (SDL_Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> tilesInRange;
  auto lambda = [this, &tilesInRange](int h, int i, int j)
  {
    std::shared_ptr<TerrainObject> terrainObjectAtCoordinates = terrainMap[h][{i, j}];
    if (terrainObjectAtCoordinates)
    {
      tilesInRange[h][terrainObjectAtCoordinates->tileType->name] += 1;
    }
  };
  iterateOverChunk(rangeRect, lambda);
  return std::make_shared<std::map<int, std::map<std::string, int>>>(tilesInRange);
}



std::map<std::string, std::map<std::string, int>> GameEngine::getCountsInRange (SDL_Rect* r)
{
  std::map<std::string, std::map<std::string, int>> results;

  auto lambda = [this, &results](int h, int i, int j)
  {
    auto t = getTerrainObjectAt(h, i, j);
    if (t->get())
    {
      if (t->get()->getTerrainType()->get())
      {
        results["terrain"][t->get()->tileType->name] += 1;
      }
      results["biome"][t->get()->biomeType->name] += 1;
    }
  };
  iterateOverChunk(r, lambda);
  return results;
}

std::shared_ptr<std::map<int, std::map<std::string, int>>> GameEngine::getBiomesInRange (SDL_Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> getBiomesInRange;
  auto lambda = [this, &getBiomesInRange](int h, int i, int j)
  {
    auto terrainObjectAtCoordinates = terrainMap[h][{i, j}];
    if (terrainObjectAtCoordinates)
    {
      getBiomesInRange[h][terrainObjectAtCoordinates->biomeType->name] += 1;
    }
  };
  iterateOverChunk(rangeRect, lambda);
  return std::make_shared<std::map<int, std::map<std::string, int>>>(getBiomesInRange);
}


template <typename F>
void GameEngine::iterateOverChunk(SDL_Rect* chunkRect, F f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    zMaxLevel, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
  );
  
  if (std::rand() % 100 > 50)
  {
    for (auto h = 0; h < zMaxLevel; h++)
    {
      for (auto i = chunkRect->x; i != chunkRect->w; i++)
      {
        for (auto j = chunkRect->y; j != chunkRect->h; j++)
        {
          f(h, i ,j);
        }
      }
    }
  }
  else
  {
    for (auto h = 0; h < zMaxLevel; h++)
    {
      for (auto j = chunkRect->y; j != chunkRect->h; j++)
      {
        for (auto i = chunkRect->x; i != chunkRect->w; i++)
        {
          f(h, i ,j);
        }
      }
    }
  }


}

std::map<int, std::vector<SDL_Point>> GameEngine::getAllPointsInRect(SDL_Rect* r)
{
  std::map<int, std::vector<SDL_Point>> results;
  for (auto h = 0; h < zMaxLevel; h++)
  {
    std::vector<SDL_Point> points;
    for (auto i = r->x; i != r->w; i++)
    {
      for (auto j = r->y; j != r->h; j++)
      {
        SDL_Point p = { i, j };
        points.push_back(p);
      }
    }
    results[h] = points;
  }
  return results;
}

template <typename F>
void GameEngine::randomlyAccessAllTilesInChunk(SDL_Rect* chunkRect, F f)
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "Processing chunk: on %d levels from ( %d, %d ) to ( %d, %d )",
    zMaxLevel, chunkRect->x, chunkRect->y, chunkRect->w, chunkRect->h
  );
  auto coordinates = getAllPointsInRect(chunkRect);
  for (auto h = 0; h < zMaxLevel; h++)
  {
    while (coordinates[h].size())
    {
      int i = std::rand() % coordinates[h].size();
      SDL_Point p = coordinates[h].at(i);
      f(h, p.x, p.y);
      coordinates[h].erase(coordinates[h].begin() + i);
    }
  }
}

int GameEngine::renderCopySprite(std::string spriteName, int x, int y)
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


template<class T>
int GameEngine::renderCopySpriteFrom(std::shared_ptr<T> t, int x, int y)
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


void GameEngine::applyUi()
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


void GameEngine::scrollGameSurface(int directions)
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


void GameEngine::handleEvents()
{
    auto lambda = [this](int directions)
    {
      std::thread th1([this](int d) { processMap(d); }, directions);
      std::thread th2([this](int d) { scrollCamera(d); }, directions);
      th1.detach();
      th2.join();
    };
    userInputHandler.handleKeyboardMovement(lambda);
  SDL_PollEvent(&appEvent);
  if (appEvent.type == SDL_QUIT)
  {
    running = false;
  }
  else if (appEvent.type == SDL_KEYDOWN)
  {
    std::shared_ptr<TerrainObject> currentTerrainObject = terrainMap[zLevel][{camera.x, camera.y}];
    switch(appEvent.key.keysym.sym)
    {
      case SDLK_ESCAPE:
        running = false;
        break;
      case SDLK_SPACE:
        SDL_Log(
          "\nCamera: %dx%dx%dx%d\nCurrent terrain object: %s\nCurrent biome: %s",
          camera.x,
          camera.y,
          camera.w,
          camera.h,
          currentTerrainObject->tileType->name.c_str(),
          currentTerrainObject->biomeType->name.c_str()
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
        if (std::abs(zLevel) < static_cast <int>(terrainMap.size()))
        {
          if (zLevel < zMaxLevel - 1)
          {
            zLevel++;
          }
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


void GameEngine::renderCopyTiles()
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
      std::vector<std::shared_ptr<WorldObject>> objects;
      std::shared_ptr<TerrainObject> t = terrainMap[zLevel][{i, j}];
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
        renderCopySpriteFrom<TerrainObject>(t, x, y);
      }
      else
      {
        renderCopySprite("Sprite 0x128", x, y);
        SDL_Rect fillChunkRect = {i - gameSize, j - gameSize, i + gameSize, j + gameSize};
        generateMapChunk(&fillChunkRect);
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


int GameEngine::renderCopyPlayer()
{
  player.x = camera.x;
  player.y = camera.y;
  auto gridSize = getWindowGridSize();
  SDL_Rect playerRect = { gridSize.first/2*tileSize, gridSize.second/2*tileSize, tileSize, tileSize };
  return SDL_RenderFillRect(appRenderer, &playerRect);
}


int GameEngine::run ()
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