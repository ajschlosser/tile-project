#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "json/json.h"
#include "objects.h"
#include "input.h"
#include "map.h"
#include "graphics.h"

#include <cmath>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <fstream>
#include <memory>
#include <mutex>

struct GameEngine
{
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
  std::map<std::string, ObjectType> objectTypes;
  std::map<std::string, BiomeType> biomeTypes;
  std::vector<std::string> biomeTypeKeys;
  std::map<std::string, TerrainType> terrainTypes;
  std::map<std::string, TileType> tileTypes;
  std::map<int, std::map<std::pair<int, int>, TerrainObject>> terrainMap;
  std::map<int, std::map<int, std::map<std::pair<int, int>, std::shared_ptr<WorldObject>>>> objectMap;
  std::map<std::string, Sprite> sprites;
  SDL_Rect camera;
  int init();
  std::pair<int, int> getWindowGridSize();
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
  GameEngine() : spriteSize(32), running(true), zLevel(0), movementSpeed(8), gameSize(50), zMaxLevel(4) {}
};

#endif