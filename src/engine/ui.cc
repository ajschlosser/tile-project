#include "engine/ui.h"

using namespace controller;

void UIController::createUIWindow (std::tuple<int, int, int, int> dimensions)
{
  auto [x, y, w, h] = dimensions;
  SDL_Color borderColor = { 255, 255, 255, 255 };
  SDL_Color bgColor = { 0, 0, 255, 255 };
  UIRect window = { { x, y, w, h }, 5, borderColor, bgColor, borderColor };
  windows.push_back(window);
}