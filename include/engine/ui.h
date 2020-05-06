#ifndef GAME_ENGINE_UI_H
#define GAME_ENGINE_UI_H

#include "engine.h"

struct UIRect : SDL_Rect
{
  int borderWidth;
  SDL_Color foregroundColor;
  SDL_Color borderColor;
  SDL_Color backgroundColor;
  SDL_Color textColor;
  std::string title;
  std::string content;
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
  void createUIWindow (int, int, int, int, std::string = "", std::string = "");
};

#endif