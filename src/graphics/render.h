#include "../graphics.h"

using namespace graphics;

int GraphicsController::renderCopySprite(Sprite *s, std::tuple<int, int, int, int> coords)
{
  auto [x, y, o_x, o_y] = coords;
  int tS = (*tileSize);
  SDL_Rect src {s->tileMapX, s->tileMapY, (*spriteSize), (*spriteSize)};
  SDL_Rect dest {(x*tS)+o_x, (y*tS)+o_y, tS, tS};
  if (SDL_RenderCopy(appRenderer, tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int GraphicsController::renderCopySprite(Sprite *s, int x, int y)
{
  int tS = (*tileSize);
  SDL_Rect src {s->tileMapX, s->tileMapY, (*spriteSize), (*spriteSize)};
  SDL_Rect dest {x*tS, y*tS, tS, tS};
  if (SDL_RenderCopy(appRenderer, tilemapTexture, &src, &dest) < -1)
  {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't copy sprite to renderer: %s", SDL_GetError());
    return 3;
  }
  return 0;
}

int GraphicsController::renderCopySprite(std::string spriteName, int x, int y)
{
  auto it = sprites.find(spriteName);
  if (it != sprites.end())
    return renderCopySprite(&it->second, x, y);
  else
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not render sprite: %s", spriteName.c_str());
    return -1;
}

int GraphicsController::renderCopyObject(std::shared_ptr<WorldObject> t, int x, int y)
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

int GraphicsController::renderCopyMobObject(std::shared_ptr<MobObject> t, int x, int y)
{
  int o_x, o_y = 0;
  if (t->relativeX != 0 || t->relativeY != 0)
  {
    o_x = t->relativeX;
    o_y = t->relativeY;
    if (t->relativeX > 0) t->relativeX -= std::floor((*tileSize)/8);
    if (t->relativeX < 0) t->relativeX += std::floor((*tileSize)/8);
    if (t->relativeY > 0) t->relativeY -= std::floor((*tileSize)/8);
    if (t->relativeY < 0) t->relativeY += std::floor((*tileSize)/8);
  }
  // TODO: UP and LEFT need different handling
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

int GraphicsController::renderCopyTerrain(TerrainObject* t, int x, int y) {
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