#include "engine/ui.h"

using namespace controller;

void TextBox::getLines (std::vector<std::string>* l)
{
  auto process = [this, l](int r = 0)
  {
    std::string offsetString = content.substr(r) + " ";
    SDL_Log("Processing string: '%s'", offsetString.c_str());
    int lineWidth, lineHeight;
    TTF_SizeUTF8(font, offsetString.c_str(), &lineWidth, &lineHeight);
    int totalLines = 1 + std::floor(lineWidth / this->w);
    int lineLen = offsetString.length() / totalLines;
    int i = 0;
    int offset = 0;
    SDL_Log("Calculations:\nString width: %d\nTextBox width: %d\nLines necessary: %d\nMax length of each line: %d",  lineWidth, this->w, totalLines, lineLen);
    while (i < totalLines)
    {
      int a = 0;
      int b = offset;
      int lineIndex = (i*lineLen)+b;
      if (lineIndex < 0)
      {
        lineIndex = 0;
      }
      std::string line;
      try
      {
        line = offsetString.substr(lineIndex, lineLen);
      }
      catch (const std::out_of_range& err)
      {
        line = offsetString;
      }
      if (line.find(" ") == std::string::npos)
      {
        line += " ";
      }
      // Calculate word-break
      while (line.length() && line.substr(line.length()-1) != " ")
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
      SDL_Log("Created line: '%s'", line.c_str());
      l->push_back(line);
      ++i;
    }
    SDL_Log("Remainder index: %d", r);
    return r;
  };
  int remainderIndex = process();
  SDL_Log("remainderIndex: %d", remainderIndex);
  if (remainderIndex > 0)
    while (remainderIndex != process(remainderIndex))
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Processing line. remainderIndex: %d", remainderIndex);
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