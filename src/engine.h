#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "objects.h"
#include "config.h"
#include "input.h"
#include "map.h"
#include "graphics.h"

#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <memory>

struct GameEngine
{
  ConfigurationController configController;
  GraphicsController gfxController;
  UserInputHandler userInputHandler;
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
  int gameSize;
  const int spriteSize;
  int zLevel;
  int zMaxLevel;
  MapController mapController;
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
  std::map<std::string, Sprite*> sprites;
  SDL_Rect camera;
  int init();
  void scrollGameSurface(int);
  std::map<int, std::map<std::string, int>> getTilesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, int>> getBiomesInRange (SDL_Rect*);
  std::map<int, std::map<std::string, std::map<std::string, int>>> getCountsInRange (SDL_Rect*);
  int generateMapChunk(SDL_Rect*);
  void processMap(int);
  void renderCopyTiles();
  void scrollCamera(int);
  void handleEvents();
  int renderCopyPlayer();
  int run();
  void iterateOverTilesInView (std::function<void(std::tuple<int, int, int, int>)> f)
  {
    auto [_w, _h] = gfxController.getWindowGridDimensions();
    int x = 0;
    int y = 0;
    for (auto i = gfxController.camera.x - _w/2; i < gfxController.camera.x + _w/2 + 5; i++)
    {
      for (auto j = gfxController.camera.y - _h/2; j < gfxController.camera.y + _h/2 + 5; j++) {
        std::tuple<int, int, int, int> locationData = {x, y, i, j};
        f(locationData);
        y++;
      }
      y = 0;
      x++;
    }
  }
  GameEngine() : tileSize(32), spriteSize(32), running(true), zLevel(0), movementSpeed(8), gameSize(200), zMaxLevel(2) {}
};

#endif