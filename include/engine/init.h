#ifndef GAME_ENGINE_INIT_H
#define GAME_ENGINE_INIT_H

#include "engine/camera.h"
#include "engine.h"

int GameEngine::init()
{
  auto cameraController = camera::CameraController(this);
  registerController<camera::CameraController>(cameraController);
  gfxController.tileSize = &tileSize;
  gfxController.spriteSize = const_cast<int*>(&spriteSize);

  std::srand(std::time(nullptr));
  if (!tileSize)
    tileSize = spriteSize;
  if (movementSpeed > tileSize)
    movementSpeed = tileSize;

  gfxController.initializeSDL();

  // Create app window and renderer
  appWindow = SDL_CreateWindow(
    "tile-project", 0, 0, gfxController.displayMode.w/2, gfxController.displayMode.h/2, SDL_WINDOW_RESIZABLE
  ); // SDL_WINDOW_FULLSCREEN
  if (appWindow == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create app window: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Window created.");
  gfxController.appWindow = appWindow;
  appRenderer = SDL_CreateRenderer(appWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC); // SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
  if (appRenderer == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create a enderer: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Renderer created.");
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
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture from surface: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Tilemap loaded.");
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
      SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Created sprite: %s", name.c_str());
    }
  }
  gfxController.sprites = spriteMap;
  SDL_Log("Spritesheet processed.");

  SDL_Log("Reading tilemap configuration file and creating tiles from sprites.");
  configController = config::ConfigurationController("tilemap.config.json", spriteMap);
  auto [biomeTypes, biomeTypeKeys, terrainTypes, mobTypes, objectTypes, tileTypes] = configController.getTypeMaps();
  player = {configController.gameSize/2, configController.gameSize/2, &configController.tileTypes["water"]};
  mapController = map::MapController(
    zMaxLevel, mobTypes, objectTypes, biomeTypes, biomeTypeKeys, terrainTypes, tileTypes, &configController
  );

  // Create default tilemap
  SDL_Log("Generating default tilemap...");
  Rect initialChunk = { 0 - configController.gameSize, 0 - configController.gameSize, configController.gameSize, configController.gameSize };
  mapController.generateMapChunk(&initialChunk);

  SDL_Log("Tilemap created.");

  return 0;
}

#endif