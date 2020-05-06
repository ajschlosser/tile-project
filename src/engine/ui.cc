#include "engine/ui.h"

using namespace controller;

void UIController::createUIWindow (int x, int y, int w, int h, std::string title, std::string content)
{
  SDL_Color fgColor = { 255, 255, 255 };
  SDL_Color borderColor = { 255, 255, 255, 255 };
  SDL_Color bgColor = { 0, 0, 255, 255 };
  UIRect window = { { x, y, w, h }, 5, fgColor, borderColor, bgColor, borderColor, title, content };
  windows.push_back(window);
}