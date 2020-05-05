#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "objects.h"
#include "rect.h"
#include "config.h"
#include "input.h"
#include "map.h"
#include "graphics.h"

#include <cmath>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <thread>
#include <memory>

namespace engine
{
  namespace graphics
  {
    template <typename T1> T1 controller;
    struct RenderController;
  }
  template <typename T1> T1 controller;
}

namespace controller
{
  struct CameraController;
  struct EventsController;
  struct MovementController;
  struct RenderController;
  struct GraphicsController;
}

struct GameEngine
{
  template <typename T> void registerController (T t)
  {
    engine::controller<T> = t;
  };
  config::ConfigurationController configController;
  graphics::GraphicsController gfxController;
  input::UserInputHandler userInputHandler;
  bool running;
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
  const int spriteSize;
  int zLevel;
  const int zMaxLevel;
  map::MapController mapController;
  objects::mobTypesMap* mobTypes;
  objects::objectTypesMap* objectTypes;
  objects::biomeTypesMap* biomeTypes;
  std::vector<std::string>* biomeTypeKeys;
  objects::terrainTypesMap* terrainTypes;
  objects::tileTypesMap* tileTypes;
  objects::biomeMap* biomeMap;
  objects::terrainMap* terrainMap;
  objects::worldMap* worldMap;
  objects::mobMap* mobMap;
  std::map<int, std::vector<std::shared_ptr<MobObject>>> mobs;
  std::map<std::string, Sprite>* sprites;
  SDL_Rect camera;
  int init();
  std::map<int, std::map<std::string, int>> getTilesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, int>> getBiomesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (SDL_Rect*);
  int generateMapChunk(SDL_Rect*);
  int run();
  bool stopRunning() { running = false; return !running; }
  int getSpriteSize() { return spriteSize; }
  int getTileSize() { return tileSize; }
  template <typename T> T* controller() { return &engine::controller<T>; }
  GameEngine() : tileSize(32), spriteSize(32), running(true), zLevel(0), movementSpeed(8), zMaxLevel(2) {}
};

#endif