#ifndef GAME_ENGINE_UI_H
#define GAME_ENGINE_UI_H

#include "engine.h"

struct Element
{
  int offsetX;
  int offsetY;
};

struct Button : Element
{
  std::function<void()> onClick;
};

struct TextBox : Element
{
  std::string content;
};

struct UIRect : SDL_Rect
{
  int borderWidth;
  SDL_Color foregroundColor;
  SDL_Color borderColor;
  SDL_Color backgroundColor;
  SDL_Color textColor;
  std::string title;
  std::string content;
  std::vector<Button> buttons;
  std::vector<TextBox> textBoxes;
  void addTextBox (TextBox t) { textBoxes.push_back(t); }
  void addButton (Button b) { buttons.push_back(b); }
  void getLines (std::vector<std::string>*, TTF_Font*);
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