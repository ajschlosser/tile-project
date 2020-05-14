#include "engine/ui.h"

using namespace controller;

void TextBox::getLines (std::vector<std::string>* l)
{
  auto process = [this, l](int r = 0)
  {
    std::string offsetString = content.substr(r) + " ";
    int lineWidth, lineHeight;
    TTF_SizeUTF8(font, offsetString.c_str(), &lineWidth, &lineHeight);
    SDL_Log("%d", this->w);
    int totalLines = 1 + lineWidth / this->w;
    int lineLen = offsetString.length() / totalLines;
    int i = 0;
    int offset = 0;
    while (i < totalLines)
    {
      int a = 0;
      int b = offset;
      int lineIndex = (i*lineLen)+b;
      if (lineIndex < 0) lineIndex = 0;
      std::string line = offsetString.substr(lineIndex, lineLen);
      // Calculate word-break
      while (line.substr(line.length()-1) != " ")
      {
        a--;
        offset--;
        int reducedLineLength = lineLen + a;
        if (lineIndex + reducedLineLength > offsetString.length())
        {
          reducedLineLength = offsetString.length() - lineIndex;
        }
        r = lineIndex + reducedLineLength;
        line = offsetString.substr(lineIndex, reducedLineLength);
      }
      l->push_back(line);
      ++i;
    }
    return r;
  };
  int remainderIndex = process();
  while (remainderIndex != process(remainderIndex))
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Processing line");
}

void UIRect::getLines (std::vector<std::string>* l, TTF_Font* f)
{
  auto process = [this, l, f](int r = 0)
  {
    std::string offsetString = content.substr(r) + " ";
    int lineWidth, lineHeight;
    TTF_SizeUTF8(f, offsetString.c_str(), &lineWidth, &lineHeight);
    int totalLines = 1 + lineWidth / w;
    int lineLen = offsetString.length() / totalLines;
    int i = 0;
    int offset = 0;
    while (i < totalLines)
    {
      int a = 0;
      int b = offset;
      int lineIndex = (i*lineLen)+b;
      if (lineIndex < 0) lineIndex = 0;
      std::string line = offsetString.substr(lineIndex, lineLen);
      // Calculate word-break
      while (line.substr(line.length()-1) != " ")
      {
        a--;
        offset--;
        int reducedLineLength = lineLen + a;
        if (lineIndex + reducedLineLength > offsetString.length())
        {
          reducedLineLength = offsetString.length() - lineIndex;
        }
        r = lineIndex + reducedLineLength;
        line = offsetString.substr(lineIndex, reducedLineLength);
      }
      l->push_back(line);
      ++i;
    }
    return r;
  };
  int remainderIndex = process();
  while (remainderIndex != process(remainderIndex))
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Processing line");
}

void UIController::createUIWindow (int x, int y, int w, int h, std::string title, std::string content)
{
  SDL_Color fgColor = { 255, 255, 255 };
  SDL_Color borderColor = { 255, 255, 255, 255 };
  SDL_Color bgColor = { 0, 0, 255, 255 };
  UIRect window = { { x, y, w, h }, 5, fgColor, borderColor, bgColor, borderColor, title, content };
  windows.push_back(window);
}

UIRect* UIController::createUIWindow (int x, int y, int w, int h)
{
  SDL_Color fgColor = { 255, 255, 255 };
  SDL_Color borderColor = { 255, 255, 255, 255 };
  SDL_Color bgColor = { 0, 0, 255, 255 };
  UIRect window = { { x, y, w, h }, 5, fgColor, borderColor, bgColor, borderColor };
  windows.push_back(window);
  return &windows.back();
}

UIRect* UIRect::setDimensions(int x, int y, int w, int h)
{
  this->x = x;
  this->y = y;
  this->w = w;
  this->h = h;
  return this;
}