#include "engine.h"
#include "engine/camera.h"
#include "engine/movement.h"
#include "engine/events.h"
#include "engine/render.h"
#include "engine/graphics.h"

int GameEngine::init()
{
  auto cameraController = controller::CameraController(this);
  auto eventsController = controller::EventsController(this);
  auto movementController = controller::MovementController(this);
  auto renderController = controller::RenderController(this);
  auto graphicsController = controller::GraphicsController(this);
  auto uiController = controller::UIController(this);
  registerController<controller::CameraController>(cameraController);
  registerController<controller::EventsController>(eventsController);
  registerController<controller::MovementController>(movementController);
  registerController<controller::RenderController>(renderController);
  registerController<controller::GraphicsController>(graphicsController);
  registerController<controller::UIController>(uiController);

  std::srand(std::time(nullptr));
  if (!tileSize)
    tileSize = spriteSize;
  if (movementSpeed > tileSize)
    movementSpeed = tileSize;

  // Create app window and renderer
  appWindow = SDL_CreateWindow(
    "tile-project", 0, 0, engine::controller<controller::GraphicsController>.displayMode.w/2, engine::controller<controller::GraphicsController>.displayMode.h/2, SDL_WINDOW_RESIZABLE
  ); // SDL_WINDOW_FULLSCREEN
  if (appWindow == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not create app window: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Window created.");
  engine::controller<controller::GraphicsController>.appWindow = appWindow;
  appRenderer = SDL_CreateRenderer(appWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC); // SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC
  if (appRenderer == NULL)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create a enderer: %s", SDL_GetError());
    return 3;
  }
  else
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Renderer created.");
  SDL_SetRenderDrawBlendMode(appRenderer, SDL_BLENDMODE_BLEND);
  gameTexture = SDL_CreateTexture(appRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, engine::controller<controller::GraphicsController>.windowWidth, engine::controller<controller::GraphicsController>.windowHeight);
  SDL_SetTextureBlendMode(gameTexture, SDL_BLENDMODE_BLEND);
  engine::controller<controller::GraphicsController>.appRenderer = appRenderer;

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

  gameFont = TTF_OpenFont("assets/FifteenNarrow.ttf", 16);
  if (!gameFont)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_OpenFont error: %s", TTF_GetError());
    return 3;    
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
  engine::controller<controller::GraphicsController>.tilemapImage = &tilemapImage;
  engine::controller<controller::GraphicsController>.tilemapTexture = texture;
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
  engine::controller<controller::GraphicsController>.sprites = spriteMap;
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
  auto UI = &engine::controller<controller::UIController>;
  // UI->createUIWindow(0, 0, 300, 300, "TEST WINDOW 1", "The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. End.");
  // UI->createUIWindow(100, 100, 600, 300, "TEST WINDOW 2", "The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. End.");
  // UI->createUIWindow(100, 200, 200, 600, "TEST WINDOW 3", "The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. End.");
  // UI->createUIWindow(300, 300, 200, 200, "TEST WINDOW 4", "The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. The lazy fox overjumped the quick brown dog. End.");
  UI->createUIWindow(15, 15, 360 , 360, "White House names heads of 'warp speed' coronavirus vaccine effort", "The Department of Health and Human Services division tasked with vaccine development, BARDA, has seen its leadership in flux after the former director Dr. Rick Bright was ousted. Bright filed a formal whistleblower complaint on Tuesday alleging his early warnings about coronavirus were ignored that that his concerns about promoting an untested therapeutic heralded by the President led to his ouster.");


  UI->createUIWindow()
    ->setDimensions(30, 30, 125, 300)
    ->setTitle("hello! there")
    ->setFont(gameFont) //
    ->addTextBox("ok this is a test of a text box. it should only be 100 px wide so let's see some line breaks! by the way, whatever.", 10, 10, 50, 100)
    ->addTextBox("fine", 50, 250, 100, 100);

  SDL_Log("Tilemap created.");

  return 0;
}

int GameEngine::run ()
{
  init();
  while (running)
  {
    controller<controller::EventsController>()->handleEvents();
    SDL_RenderClear(appRenderer);
    controller<controller::RenderController>()->renderCopyTiles();
    controller<controller::RenderController>()->renderCopyPlayer();
    engine::controller<controller::GraphicsController>.applyUI();
    engine::controller<controller::RenderController>.renderUI();
    SDL_RenderPresent(appRenderer);
  }
  return 1;
}