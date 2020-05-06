#include "engine/graphics.h"
#include "engine/graphics/render.h"

using namespace engine::graphics;

int RenderController::renderCopySprite(Sprite *s, std::tuple<int, int, int, int> coords)
{
  auto [x, y, o_x, o_y] = coords;
  int tS = e->getTileSize();
  SDL_Rect src {s->tileMapX, s->tileMapY, e->getSpriteSize(), e->getSpriteSize()};
  SDL_Rect dest {(x*tS)+o_x, (y*tS)+o_y, tS, tS};
  if (SDL_RenderCopy(e->appRenderer, engine::controller<controller::GraphicsController>.tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int RenderController::renderCopySprite(Sprite *s, int x, int y)
{
  int tS = e->getTileSize();
  SDL_Rect src {s->tileMapX, s->tileMapY, e->getSpriteSize(), e->getSpriteSize()};
  SDL_Rect dest {x*tS, y*tS, tS, tS};
  if (SDL_RenderCopy(e->appRenderer, engine::controller<controller::GraphicsController>.tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int RenderController::renderCopySprite(std::string spriteName, int x, int y)
{
  auto it = e->sprites->find(spriteName);
  if (it != e->sprites->end())
    return renderCopySprite(&it->second, x, y);
  else
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not render sprite: %s", spriteName.c_str());
    return -1;
}

int RenderController::renderCopyObject(std::shared_ptr<WorldObject> t, int x, int y)
{
  if (!t->isAnimated())
    return renderCopySprite(t->objectType->getFrame(0), x, y);
  else
  {
    if (t->animationTimer.elapsed() > t->animationSpeed)
    {
      t->animationTimer.stop();
      t->animationTimer.start();
      t->animationFrame++;
      if (t->animationFrame >= t->objectType->maxFrames())
        t->animationFrame = 0;
    }
    auto it = t->objectType->animationMap[t->direction].find(t->animationFrame);
    if (it == t->objectType->animationMap[t->direction].end())
      return renderCopySprite(t->objectType->getFrame(0), x, y);
    else
      return renderCopySprite(it->second, x, y);
  }
}

int RenderController::renderCopyMobObject(std::shared_ptr<MobObject> t, int x, int y)
{
  int o_x, o_y = 0;
  if (t->relativeX != 0 || t->relativeY != 0)
  {
    o_x = t->relativeX;
    o_y = t->relativeY;
    if (t->relativeX > 0) t->relativeX -= std::floor(e->getTileSize()/8);
    if (t->relativeX < 0) t->relativeX += std::floor(e->getTileSize()/8);
    if (t->relativeY > 0) t->relativeY -= std::floor(e->getTileSize()/8);
    if (t->relativeY < 0) t->relativeY += std::floor(e->getTileSize()/8);
  }
  if (!t->isAnimated())
    return renderCopySprite(t->mobType->getFrame(0), x, y);
  else
  {
    if (t->animationTimer.elapsed() > t->animationSpeed)
    {
      t->animationTimer.stop();
      t->animationTimer.start();
      t->animationFrame++;
      if (t->animationFrame >= t->mobType->maxFrames(t->direction))
        t->animationFrame = 0;
    }
    auto it = t->mobType->animationMap[t->direction].find(t->animationFrame);
    if (it == t->mobType->animationMap[t->direction].end())
      return renderCopySprite(t->mobType->getFrame(0), { x, y, o_x, o_y });
    else
      return renderCopySprite(it->second, { x, y, o_x, o_y });
  }
}

int RenderController::renderCopyTerrain(TerrainObject* t, int x, int y) {
  if (!t->isAnimated())
    return renderCopySprite(t->terrainType->getFrame(0), x, y);
  else
  {
    if (t->animationTimer.elapsed() > t->animationSpeed)
    {
      t->animationTimer.stop();
      t->animationTimer.start();
      t->animationFrame++;
      if (t->animationFrame >= t->terrainType->maxFrames())
        t->animationFrame = 0;
    }
    auto it = t->terrainType->animationMap[t->direction].find(t->animationFrame);
    if (it == t->terrainType->animationMap[t->direction].end())
      return renderCopySprite(t->terrainType->getFrame(0), x, y);
    else
      return renderCopySprite(it->second, x, y);
  }
}

int RenderController::renderFillUIWindow(UIRect* window)
{
  SDL_SetRenderDrawColor(
    engine::controller<controller::GraphicsController>.appRenderer,
    window->backgroundColor.r,
    window->backgroundColor.g,
    window->backgroundColor.b,
    window->backgroundColor.a
  );
  SDL_Rect r {window->x, window->y, window->w, window->h};
  SDL_RenderFillRect(
    engine::controller<controller::GraphicsController>.appRenderer,
    &r
  );
  SDL_Surface* title = TTF_RenderText_Solid(e->gameFont, window->title.c_str(), window->foregroundColor);
  SDL_Texture* tex = SDL_CreateTextureFromSurface(e->appRenderer, title);
  if (!title)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "TTF_RenderText_Solid error: %s", TTF_GetError());
  }
  else
  {
    SDL_Rect titleRect {window->x+5, window->y+5, title->w, title->h};
    SDL_RenderCopy(e->appRenderer, tex, NULL, &titleRect);
  }

  std::vector<std::string> lines;
  int _w, _h;
  TTF_SizeUTF8(e->gameFont,window->content.c_str(),&_w,&_h);
  int nLines = 1 + _w / window->w;
  int lineLen = static_cast<int>(window->content.length()) / nLines;

  int i = 0;
  int a = 0;
  window->content = " " + window->content + " ";
  while (i < nLines)
  {
    int b = a;
    std::string line = window->content.substr((i*lineLen)+b, lineLen);
    //a = 0;
    while (line.substr(line.length()-1) != " ")
    {
      a--;
      line = window->content.substr((i*lineLen)+b, lineLen + a);
      // SDL_Log("better line: %s", line.c_str());
    }
    lines.push_back(line);
    ++i;
  }


  i = 0;
  for ( auto l : lines )
  {
    SDL_Surface* t = TTF_RenderText_Solid(e->gameFont, l.c_str(), window->foregroundColor);
    SDL_Texture* txt = SDL_CreateTextureFromSurface(e->appRenderer, t);
    SDL_Rect titleRect {window->x+15, window->y+15+_h*i+5, t->w, t->h};
    SDL_RenderCopy(e->appRenderer, txt, NULL, &titleRect);
    SDL_FreeSurface(t);
    SDL_DestroyTexture(txt);
    ++i;
  }

  SDL_FreeSurface(title);
  SDL_DestroyTexture(tex);
  return 0;
}