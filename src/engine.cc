#include "engine.h"

int GameEngine::init()
{
  gfxController.tileSize = &tileSize;
  gfxController.spriteSize = const_cast<int*>(&spriteSize);

  std::srand(std::time(nullptr));
  if (!tileSize)
    tileSize = spriteSize;
  if (movementSpeed > tileSize)
    movementSpeed = tileSize;

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
  appRenderer = SDL_CreateRenderer(appWindow, -1, SDL_RENDERER_PRESENTVSYNC); // SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
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
  gameTexture = SDL_CreateTexture(appRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, gfxController.windowWidth, gfxController.windowHeight);
  SDL_SetTextureBlendMode(gameTexture, SDL_BLENDMODE_BLEND);
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
  std::map<std::string, Sprite> spriteMap;
  for (auto i = 0; i < surface->w; i += spriteSize)
  {
    for (auto j = 0; j < surface->h; j += spriteSize)
    {
      std::string name = "Sprite " + std::to_string(i) + "x" + std::to_string(j);
      Sprite s { i, j, name };
      spriteMap[name] = s;
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
        "Created sprite: %s",
        name.c_str()
      );
    }
  }
  gfxController.sprites = spriteMap;
  SDL_Log("Spritesheet processed.");

  SDL_Log("Reading tilemap configuration file and creating tiles from sprites.");
  configController = ConfigurationController("tilemap.config.json", spriteMap);
  auto [biomeTypes, biomeTypeKeys, terrainTypes, mobTypes, objectTypes, tileTypes] = configController.getTypeMaps();
  player = {configController.gameSize/2, configController.gameSize/2, &configController.tileTypes["water"]};
  mapController = MapController(
    zMaxLevel, mobTypes, objectTypes, biomeTypes, biomeTypeKeys, terrainTypes, tileTypes, &configController
  );

  // Create default tilemap
  SDL_Log("Generating default tilemap...");
  Rect initialChunk = { 0 - configController.gameSize, 0 - configController.gameSize, configController.gameSize, configController.gameSize };
  mapController.generateMapChunk(&initialChunk);

  SDL_Log("Tilemap of %d tiles created.",
    configController.gameSize*configController.gameSize*4
  );

  return 0;
}

void GameEngine::scrollCamera(int directions)
{
  if (tileSize > 8)
    scrollGameSurface(directions);
  else
    SDL_Delay(25);
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
    gfxController.camera.x-configController.gameSize*2,
    gfxController.camera.y-configController.gameSize*2,
    gfxController.camera.x+configController.gameSize*2,
    gfxController.camera.y+configController.gameSize*2
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
    // SDL_SetRenderTarget(appRenderer, gameTexture);
    renderCopyTiles();
    if (directions & RIGHT)
      dest.x -= movementSpeed;
    if (directions & LEFT)
      dest.x += movementSpeed;
    if (directions & UP)
      dest.y += movementSpeed;
    if (directions & DOWN)
      dest.y -= movementSpeed;

    SDL_RenderCopy(appRenderer, gfxController.getGameSurfaceTexture(), NULL, &dest);
    renderCopyPlayer();
    gfxController.applyUi();
    // SDL_SetRenderTarget(appRenderer, NULL);
    // SDL_RenderCopy(appRenderer, gameTexture, &dest, NULL);
    SDL_RenderPresent(appRenderer);
    // SDL_DestroyTexture(gameTexture);
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
      SDL_Delay(1500);
      running = false;
    }
    else if (event->type == SDL_KEYDOWN)
    {
      auto t = &mapController.terrainMap[zLevel][{gfxController.camera.x, gfxController.camera.y}];
      std::string objs;
      switch(event->key.keysym.sym)
      {
        case SDLK_ESCAPE:
          SDL_Delay(1500);
          running = false;
          break;
        case SDLK_SPACE:
          SDL_Log(
            "\nCamera: %dx%dx%dx%d\nCurrent terrain type: %s\nCurrent biome type: %s\nCurrent terrain type sprite name: %s\nInitialized: %d",
            gfxController.camera.x,
            gfxController.camera.y,
            gfxController.camera.w,
            gfxController.camera.h,
            t->terrainType->name.c_str(),
            t->biomeType->name.c_str(),
            t->terrainType->sprite->name.c_str(),
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

  auto renderer = [this](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto terrainObject = mapController.terrainMap[zLevel].find({ i, j });
    auto worldObject = mapController.worldMap[zLevel].find({ i, j });
    if (terrainObject != mapController.terrainMap[zLevel].end())
      gfxController.renderCopyTerrain(&terrainObject->second, x, y);
    else
      gfxController.renderCopySprite("Sprite 0x128", x, y);
    if (worldObject != mapController.worldMap[zLevel].end())
      for ( auto w : worldObject->second )
        gfxController.renderCopyObject<WorldObject>(w, x, y);
    auto mobObject = mapController.mobMap[zLevel].find({ i, j });
    if (mobObject != mapController.mobMap[zLevel].end())
      for ( auto &w : mobObject->second )
        gfxController.renderCopyObject<MobObject>(w, x, y);
  };
  std::thread r (
    [this](std::function<void(std::tuple<int, int, int, int>)> f) { iterateOverTilesInView(f); }, renderer
  );

  auto processor = [this](std::tuple<int, int, int, int> locationData){
    auto [x, y, i, j] = locationData;
    auto it = mapController.mobMap[zLevel][{i, j}].begin();
    while (it != mapController.mobMap[zLevel][{i, j}].end())
    {
      if (it->get()->mobTimers["movement"].elapsed() > it->get()->speed)
      {
        it->get()->mobTimers["movement"].stop();
        it->get()->mobTimers["movement"].start();
        int n = std::rand() % 100;
        if (n > 75) it = mapController.moveMob(it->get()->id, {zLevel, i, j}, {zLevel, i+1, j});
        else if (n > 50) it = mapController.moveMob(it->get()->id, {zLevel, i, j}, {zLevel, i-1, j});
        else if (n > 25) it = mapController.moveMob(it->get()->id, {zLevel, i, j}, {zLevel, i, j+1});
        else it = mapController.moveMob(it->get()->id, {zLevel, i, j}, {zLevel, i, j-1});
      }
      else ++it;
    }
  };
  std::thread p (
    [this](std::function<void(std::tuple<int, int, int, int>)> f) { iterateOverTilesInView(f); }, processor
  );

  r.join();
  p.detach();

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