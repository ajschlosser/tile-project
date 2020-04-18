#include "SDL2/SDL.h"
#include "engine.h"

int GameEngine::init()
{
  player = {gameSize/2, gameSize/2, "Sprite 0x96", &tileTypes["water"], 100};
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
    std::vector<std::string> relatedObjectTypes;
    const Json::Value& relatedObjectsArray = tileConfigJson["tiles"][i]["objects"];
    for (int i = 0; i < relatedObjectsArray.size(); i++)
    {
      relatedObjectTypes.push_back(relatedObjectsArray[i].asString());
    }
    SDL_Log("- Loaded '%s' tile", tileTypeName.c_str());
    TileType t { &sprites[spriteName], tileTypeName, relatedObjectTypes };
    tileTypes[tileTypeName] = t;
  }
  for (auto i = 0; i < tileConfigJson["objects"].size(); ++i)
  {
    std::string spriteName = tileConfigJson["objects"][i]["sprite"].asString();
    std::string objectTypeName = tileConfigJson["objects"][i]["name"].asString();
    SDL_Log("- Loaded '%s' object", objectTypeName.c_str());

    TileType t { &sprites[spriteName], objectTypeName };
    tileTypes[objectTypeName] = t;
    //ObjectType o { &sprites[spriteName], objectTypeName };
    //objectTypes[objectTypeName] = o;
  }
  for (auto i = 0; i < tileConfigJson["biomes"].size(); ++i)
  {
    //std::string spriteName = tileConfigJson["objects"][i]["sprite"].asString();
    std::string biomeTypeName = tileConfigJson["biomes"][i]["name"].asString();
    BiomeType b = { biomeTypeName };
    b.maxDepth = tileConfigJson["biomes"][i]["maxDepth"].asInt();
    b.maxDepth = tileConfigJson["biomes"][i]["maxDepth"].asInt();
    const Json::Value& terrainsArray = tileConfigJson["biomes"][i]["terrains"];
    for (int i = 0; i < terrainsArray.size(); i++)
    {
      auto t = terrainsArray[i];
      b.terrainTypes[b.terrainTypes.size()] = { t["name"].asString(), t["multiplier"].asFloat() };
    }
    biomeTypes[biomeTypes.size()] = b;
    SDL_Log("- Loaded '%s' biome", biomeTypeName.c_str());

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

void GameEngine::handleEvents()
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
    std::string tileType = (*terrainMap[zLevel][{camera.x, camera.y}]).tileType->name;
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
          if (zLevel < zDepth - 1)
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