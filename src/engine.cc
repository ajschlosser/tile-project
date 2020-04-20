#include "SDL2/SDL.h"
#include "engine.h"

int GameEngine::init()
{
  player = {gameSize/2, gameSize/2, &tileTypes["water"]};

  mapController.maxDepth = zMaxLevel;
  gfxController.tileSize = &tileSize;
  gfxController.spriteSize = const_cast<int*>(&spriteSize);

  std::srand(std::time(nullptr));
  if (!tileSize)
  {
    tileSize = spriteSize;
  }
  if (movementSpeed > tileSize)
  {
    movementSpeed = tileSize;
  }

  gfxController.initializeSDL();

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
    gfxController.displayMode.w/tileSize,
    gfxController.displayMode.h/tileSize
  );

  // Create app window and renderer
  appWindow = SDL_CreateWindow("tile-project", 0, 0, gfxController.displayMode.w/2, gfxController.displayMode.h/2, SDL_WINDOW_RESIZABLE);
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
  gfxController.appRenderer = appRenderer;

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
  gfxController.tilemapImage = &tilemapImage;
  gfxController.tilemapTexture = texture;
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
  gfxController.sprites = sprites;
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
    auto b = std::make_shared<BiomeType>();
    b->name = tileConfigJson["biomes"][i]["name"].asString();
    b->maxDepth = tileConfigJson["biomes"][i]["maxDepth"].asInt();
    b->minDepth = tileConfigJson["biomes"][i]["minDepth"].asInt();
    const Json::Value& terrainsArray = tileConfigJson["biomes"][i]["terrains"];
    for (int i = 0; i < terrainsArray.size(); i++)
    {
      auto t = terrainsArray[i];
      b->terrainTypes[b->terrainTypes.size()] = { t["name"].asString(), t["multiplier"].asFloat() };
    }
    biomeTypes[b->name] = b;
    biomeTypeKeys.push_back(b->name);
    
    SDL_Log("- Loaded '%s' biome", b->name.c_str());
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
  return { width, height };
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

  //SDL_Rect ok = { camera.x-gameSize*2, camera.y-gameSize*2, camera.x+gameSize*2, camera.y+gameSize*2 };

  auto t = &terrainMap[zLevel][{ checkCoordinates.x, checkCoordinates.y }];
  if (!t->get())
  {
    SDL_Log("here: %d %d", checkCoordinates.x, checkCoordinates.y);
    generateMapChunk(&chunkRect);
  }

};


int GameEngine::generateMapChunk(SDL_Rect* chunkRect)
{

  if (mapController.mapGenerator.currentlyGenerating())
  {
    return -1;
  }

  mapController.mapGenerator.init(biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]]);
  SDL_Log("Generating chunk: %s", mapController.mapGenerator.currentBiomeType->name.c_str());

  auto createTerrainPlaceholders = [this](int h, int i, int j)
  {
    auto terrainObjectAtCoordinates = &terrainMap[h][{i, j}];
    while (mapController.mapGenerator.isOutOfDepth(h))
    {
      mapController.mapGenerator.currentBiomeType = biomeTypes[biomeTypeKeys[std::rand() % biomeTypeKeys.size()]];
    }
    auto b = mapController.mapGenerator.currentBiomeType;
    if (!terrainObjectAtCoordinates->get())
    {
      terrainMap[h][{i, j}] = std::make_shared<TerrainObject>(i, j, b);
    }
  };

  // TODO: Bad things happen if this takes too long
  auto growBiomes = [this](int h, int i, int j)
  {
    auto t = &terrainMap[h][{ i, j }];
    if (t->get())
      {
      SDL_Rect r { i-5, j-5, i+5, j+5 };
      auto counts = getCountsInRange(&r);

      std::pair<std::string, int> top;
      for (const auto &pair : counts[h]["biome"])
      {
        if (pair.second > top.second)
        {
          top.second = pair.second;
          top.first = pair.first;
        }
      }

      if (t->get()->biomeType->name != top.first)
      { 
        t->get()->biomeType = biomeTypes[top.first];
      }
    }

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

  SDL_Log("Creating terrain placeholders for biome type '%s'", mapController.mapGenerator.currentBiomeType->name.c_str());
  mapController.iterateOverChunk(chunkRect, createTerrainPlaceholders);
  SDL_Log("growing biomes");
  mapController.randomlyAccessAllTilesInChunk(chunkRect, growBiomes);
  SDL_Log("setting terrain type");
  mapController.iterateOverChunk(chunkRect, setTerrainTypes);
  SDL_Log("adding world objects");
  mapController.iterateOverChunk(chunkRect, addWorldObjects);
  mapController.mapGenerator.reset();
  SDL_Log("Created chunk. Map now has %lu tiles", terrainMap[0].size());
  return 0;
}


std::map<int, std::map<std::string, int>> GameEngine::getTilesInRange (SDL_Rect* rangeRect)
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
  mapController.iterateOverChunk(rangeRect, lambda);
  return tilesInRange;
}


std::map<int, std::map<std::string, std::map<std::string, int>>> GameEngine::getCountsInRange (SDL_Rect* r)
{
  std::map<int, std::map<std::string, std::map<std::string, int>>> results;

  auto lambda = [this, &results](int h, int i, int j)
  {
    auto t = getTerrainObjectAt(h, i, j); // TODO: This is fucked up
    if (t->get())
    {
      if (t->get()->getTerrainType()->get())
      {
        results[h]["terrain"][t->get()->tileType->name] += 1;
      }
      results[h]["biome"][t->get()->biomeType->name] += 1;
    }
  };
  mapController.iterateOverChunk(r, lambda);
  return results;
}


std::map<int, std::map<std::string, int>> GameEngine::getBiomesInRange (SDL_Rect* rangeRect)
{
  std::map<int, std::map<std::string, int>> results;
  auto lambda = [this, &results](int h, int i, int j)
  {
    auto terrainObjectAtCoordinates = terrainMap[h][{i, j}];
    if (terrainObjectAtCoordinates)
    {
      results[h][terrainObjectAtCoordinates->biomeType->name] += 1;
    }
  };
  mapController.iterateOverChunk(rangeRect, lambda);
  return results;
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
    gfxController.applyUi();
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
      //std::thread th1([this](int d) { processMap(d); }, directions);
      std::thread th2([this](int d) { scrollCamera(d); }, directions);
      //th1.detach();
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
        gfxController.renderCopySpriteFrom<TerrainObject>(t, x, y);
      }
      else
      {
        // auto nT = std::make_shared<TerrainObject>(x, y, &biomeTypes[0]);
        // nT->tileType = &tileTypes["grass"];
        // terrainMap[zLevel][{i, j}] = nT;
        // gfxController.renderCopySpriteFrom<TerrainObject>(nT, x, y);

        // TODO: If the below takes too long, bad things happen
        gfxController.renderCopySprite("Sprite 0x128", x, y);
        SDL_Rect fillChunkRect = {i - 5, j - 5, i + 5, j + 5};
        generateMapChunk(&fillChunkRect);
      }
      for (std::shared_ptr<WorldObject> o : objects)
      {
        gfxController.renderCopySpriteFrom<WorldObject>(o, x, y);
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
    gfxController.applyUi();
    SDL_RenderPresent(appRenderer);
  }
  return 1;
}