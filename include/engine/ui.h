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
  int w;
  int h;
  TTF_Font* font;
  void getLines (std::vector<std::string>*);
  TextBox (TTF_Font* f, std::string c, int x = 0, int y = 0, int w = 1, int h = 0)
  {
    font = f;
    offsetX = x;
    offsetY = y;
    content = c;
    if (!w && !h)
    {
      TTF_SizeUTF8(font, c.c_str(), &w, &h);
    }
    this->w = w;
    this->h = h;
  }
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
  TTF_Font* font;
  void getLines (std::vector<std::string>*, TTF_Font*);
  UIRect* setDimensions (int, int, int, int);
  UIRect* setTitle (std::string t) { this->title = t; return this; }
  UIRect* setFont (TTF_Font* f) { this->font = f; return this; }
  UIRect* addTextBox (std::string s, int x = 0, int y = 0, int w = 0, int h = 0) {
    TextBox t{font, s, x, y, w, h};
    textBoxes.push_back(t);
    return this;
  }
  UIRect* addButton (Button b) { buttons.push_back(b); return this; }
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
  UIRect* createUIWindow (int = 0, int = 0, int = 0, int = 0);
};

#endif