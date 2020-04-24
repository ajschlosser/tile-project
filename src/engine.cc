#include "SDL2/SDL.h"
#include "engine.h"

int GameEngine::init()
{
  player = {gameSize/2, gameSize/2, &mapController.tileTypes["water"]};
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
  gfxController.appWindow = appWindow;
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
    TileType tileType { &sprites[spriteName], tileTypeName };
    TerrainType terrainType { &sprites[spriteName], tileTypeName, relatedObjectTypes };
    tileTypes[tileType.name] = tileType;
    terrainTypes[terrainType.name] = terrainType;
  }
  for (auto i = 0; i < tileConfigJson["biomes"].size(); ++i)
  {
    BiomeType b;
    b.name = tileConfigJson["biomes"][i]["name"].asString();
    b.maxDepth = tileConfigJson["biomes"][i]["maxDepth"].asInt();
    b.minDepth = tileConfigJson["biomes"][i]["minDepth"].asInt();
    const Json::Value& terrainsArray = tileConfigJson["biomes"][i]["terrains"];
    for (int i = 0; i < terrainsArray.size(); i++)
    {
      auto t = terrainsArray[i];
      b.terrainTypes[b.terrainTypes.size()] = { t["name"].asString(), t["multiplier"].asFloat() };
    }
    biomeTypes[b.name] = b;
    biomeTypeKeys.push_back(b.name);
    
    SDL_Log("- Loaded '%s' biome", b.name.c_str());
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

    TileType tileType { &sprites[spriteName], objectTypeName };
    tileTypes[tileType.name] = tileType;

    ObjectType o { &sprites[spriteName], objectTypeName, bM };
    objectTypes[objectTypeName] = o;
  }

  mapController = MapController(
    zMaxLevel, objectTypes, biomeTypes, biomeTypeKeys, terrainTypes, tileTypes
  );

  // Create default tilemap
  SDL_Log("Generating default tilemap...");
  Rect initialChunk = { 0 - gameSize, 0 - gameSize, gameSize, gameSize };
  mapController.generateMapChunk(&initialChunk);

  SDL_Log("Tilemap of %d tiles created.",
    gameSize*gameSize*4
  );

  return 0;
}

void GameEngine::scrollCamera(int directions)
{
  if (tileSize > 8)
    scrollGameSurface(directions);
  else
    SDL_Delay(1);
  if (directions & LEFT)
    gfxController.camera.x -= 1;
  if (directions & RIGHT)
    gfxController.camera.x += 1;
  if (directions & DOWN)
    gfxController.camera.y += 1;
  if (directions & UP)
    gfxController.camera.y -= 1;
}


void GameEngine::processMap(int directions)
{
  auto [_w, _h] = gfxController.getWindowGridDimensions();
  SDL_Point checkCoordinates = { gfxController.camera.x, gfxController.camera.y };
  Rect chunkRect = {
    gfxController.camera.x-gameSize*2,
    gfxController.camera.y-gameSize*2,
    gfxController.camera.x+gameSize*2,
    gfxController.camera.y+gameSize*2
  };
  if (directions & RIGHT)
    checkCoordinates.x += _w;
  if (directions & LEFT)
    checkCoordinates.x -= _w;
  if (directions & UP)
    checkCoordinates.y -= _h;
  if (directions & DOWN)
    checkCoordinates.y += _h;
  auto it = mapController.terrainMap[zLevel].find({ checkCoordinates.x, checkCoordinates.y });
  if (it == mapController.terrainMap[zLevel].end())
  {
    SDL_Log("Detected ungenerated map: %d %d", checkCoordinates.x, checkCoordinates.y);
    mapController.generateMapChunk(&chunkRect);
  }

};


void GameEngine::scrollGameSurface(int directions)
{
  auto [_w, _h] = gfxController.getWindowDimensions();
  SDL_Rect dest {0, 0, _w, _h};
  std::pair<int, int> offset = {0, 0};
  if (directions & RIGHT)
    offset.first -= tileSize;
  if (directions & LEFT)
    offset.first += tileSize;
  if (directions & UP)
    offset.second += tileSize;
  if (directions & DOWN)
    offset.second -= tileSize;
  while (dest.x != offset.first || dest.y != offset.second)
  {
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
      "scrollGameSurface: offset: %d %d \t dest: %d %d", offset.first, offset.second, dest.x, dest.y
    );
    if (SDL_RenderClear(appRenderer) < 0)
    {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not clear renderer: %s", SDL_GetError());
      break;
    }
    renderCopyTiles();
    gameTexture = gfxController.getGameSurfaceTexture();
    if (directions & RIGHT)
      dest.x -= movementSpeed;
    if (directions & LEFT)
      dest.x += movementSpeed;
    if (directions & UP)
      dest.y += movementSpeed;
    if (directions & DOWN)
      dest.y -= movementSpeed;
    SDL_RenderCopy(appRenderer, gameTexture, NULL, &dest);
    renderCopyPlayer();
    gfxController.applyUi();
    SDL_RenderPresent(appRenderer);
    SDL_DestroyTexture(gameTexture);
  }
  if (SDL_SetRenderTarget(appRenderer, NULL) < 0)
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not reset render target: %s", SDL_GetError());
}


void GameEngine::handleEvents()
{
  auto keyboardMovementHandler = [this](int directions)
  {
    std::thread graphicalThread([this](int d) { scrollCamera(d); }, directions);
    std::thread mapProcessingThread([this](int d) { processMap(d); }, directions);
    mapProcessingThread.detach();
    graphicalThread.join();
  };
  userInputHandler.handleKeyboardMovement(keyboardMovementHandler);

  auto eventHandler = [this](SDL_Event* event)
  {
    if (event->type == SDL_QUIT)
    {
      running = false;
    }
    else if (event->type == SDL_KEYDOWN)
    {
      auto t = &mapController.terrainMap[zLevel][{gfxController.camera.x, gfxController.camera.y}];
      auto it = mapController.objectMap[zLevel].find({ gfxController.camera.x, gfxController.camera.y });
      std::string objs;
      switch(event->key.keysym.sym)
      {
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_SPACE:
          if (it != mapController.objectMap[zLevel].end())
            for (auto [a, b] : it->second)
              objs += b->objectType->name + " ";
          SDL_Log(
            "\nCamera: %dx%dx%dx%d\nCurrent tile type: %s\nCurrent terrain type: %s\nCurrent biome type: %s\nCurrent tile type sprite name: %s\nCurrent terrain type sprite name: %s\nObjects on tile: %s\nInitialized: %d",
            gfxController.camera.x,
            gfxController.camera.y,
            gfxController.camera.w,
            gfxController.camera.h,
            t->tileType->name.c_str(),
            t->terrainType->name.c_str(),
            t->biomeType->name.c_str(),
            t->tileType->sprite->name.c_str(),
            t->terrainType->sprite->name.c_str(),
            objs.c_str(),
            t->initialized
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
          if (std::abs(zLevel) < static_cast <int>(mapController.terrainMap.size()))
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
  };
  userInputHandler.handleAppEvents(eventHandler);

}

void GameEngine::renderCopyTiles()
{
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
    "renderCopyTiles() called"
  );
  auto [_w, _h] = gfxController.getWindowGridDimensions();
  int x = 0;
  int y = 0;
  for (auto i = gfxController.camera.x - _w/2; i < gfxController.camera.x + _w/2 + 5; i++)
  {
    for (auto j = gfxController.camera.y - _h/2; j < gfxController.camera.y + _h/2 + 5; j++)
    {
      std::vector<std::shared_ptr<WorldObject>> objects;
      for (auto [a, b] : mapController.objectMap[zLevel][{i, j}])
        objects.push_back(b);
      auto mapLevel = mapController.terrainMap[zLevel].find({ i, j });
      if (mapLevel != mapController.terrainMap[zLevel].end())
        gfxController.renderCopySpriteFrom<TerrainObject>(&mapLevel->second, x, y);
      else
        gfxController.renderCopySprite("Sprite 0x128", x, y);
      for (std::shared_ptr<WorldObject> o : objects)
        gfxController.renderCopySpriteFrom<WorldObject>(o, x, y);
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
  player.x = gfxController.camera.x;
  player.y = gfxController.camera.y;
  auto [_w, _h] = gfxController.getWindowGridDimensions();
  SDL_Rect playerRect = { _w/2*tileSize, _h/2*tileSize, tileSize, tileSize };
  return SDL_RenderFillRect(appRenderer, &playerRect);
}


int GameEngine::run ()
{
  init();
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