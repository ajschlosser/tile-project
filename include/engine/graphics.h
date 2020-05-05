#ifndef GAME_ENGINE_GRAPHICS_H
#define GAME_ENGINE_GRAPHICS_H

#include "engine/graphics/render.h"
#include "engine.h"
#include "objects.h"


struct controller::GraphicsController
{
  GameEngine *e;
  SDL_Window* appWindow;
  SDL_Renderer* appRenderer;
  SDL_Texture* tilemapTexture;
  Image* tilemapImage;
  SDL_DisplayMode displayMode;
  SDL_Rect camera;
  std::map<std::string, Sprite> sprites;
  int windowWidth;
  int windowHeight;
  int initializeSDL();
  std::tuple<int, int> getWindowGridDimensions();
  std::tuple<int, int> getWindowDimensions();
  void applyUi();
  int renderCopySprite(Sprite*, std::tuple<int, int, int, int>);
  int renderCopySprite(std::string, int, int);
  int renderCopySprite(Sprite*, int, int);
  int renderCopyObject(std::shared_ptr<WorldObject>, int, int);
  int renderCopyMobObject(std::shared_ptr<MobObject>, int, int);
  int renderCopyTerrain(TerrainObject*, int, int);
  SDL_Surface* getGameSurfaceFromWindow();
  SDL_Texture* getTextureFromSurface (SDL_Surface* s);
  SDL_Texture* getGameSurfaceTexture ();
  template <typename T> void registerController (T t)
  {
    engine::graphics::controller<T> = t;
  };
  template <typename T> T* controller() { return &engine::graphics::controller<T>; }
  GraphicsController () {}
  GraphicsController (GameEngine* e)
  {
    this->e = e;
    auto renderController = engine::graphics::RenderController(e);
    engine::graphics::controller<engine::graphics::RenderController> = renderController;
  }
};

#endif