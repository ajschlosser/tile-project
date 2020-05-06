#ifndef GAME_ENGINE_UI_H
#define GAME_ENGINE_UI_H

#include "engine.h"

struct UIRect : SDL_Rect
{
  int borderWidth;
  SDL_Color borderColor;
  SDL_Color backgroundColor;
  SDL_Color textColor;
};

struct controller::UIController
{
  GameEngine* e;
  std::vector<UIRect> windows;
  UIController () {}
  UIController (GameEngine* e)
  {
    this->e = e;
  }
  void createUIWindow (std::tuple<int, int, int, int> dimensions)
  {
    auto [x, y, w, h] = dimensions;
    SDL_Color borderColor = { 255, 255, 255, 255 };
    SDL_Color bgColor = { 0, 0, 255, 255 };
    UIRect window = { x, y, w, h, 5, borderColor, bgColor, borderColor };
    windows.push_back(window);
  }
};

#endif