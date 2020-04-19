#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "json/json.h"
#include "objects.h"
#include "input.h"

#include <cmath>
#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <fstream>
#include <memory>

struct Image
{
  SDL_Surface* surface;
  SDL_Texture* texture;
};

struct Sprite
{
  int tileMapX;
  int tileMapY;
  std::string tileName;
};

struct GameEngine
{
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
  bool generatingChunk;
  std::map<int, BiomeType> biomeTypes;
  std::map<std::string, TileType> tileTypes;
  std::map<std::string, ObjectType> objectTypes;
  std::map<int, std::map<std::pair<int, int>, std::shared_ptr<Tile>>> terrainMap;
  std::map<int, std::map<int, std::map<std::pair<int, int>, std::shared_ptr<WorldObject>>>> objectMap;
  std::map<std::string, Sprite> sprites;
  SDL_Rect camera;
  GameEngine() : generatingChunk(false), spriteSize(32), running(true), zLevel(0), movementSpeed(8), gameSize(50), zMaxLevel(4) {}
  int init();
  std::pair<int, int> getWindowGridSize();
  int renderCopySprite(std::string, int, int);
  template <class T>
  int renderCopySpriteFrom(std::shared_ptr<T>, int, int);
  void scrollGameSurface(int);
  void applyUi();
  template<typename F>
  void iterateOverChunk(SDL_Rect*, F);
  std::shared_ptr<std::map<int, std::map<std::string, int>>> getTilesInRange (SDL_Rect*);
  int generateMapChunk(SDL_Rect*);
  void processMap(int);
  void renderCopyTiles();
  void scrollCamera(int);
  void handleEvents();
  int renderCopyPlayer();
  int run();
};