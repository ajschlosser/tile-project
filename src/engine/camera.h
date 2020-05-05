#include "../engine.h"

void GameEngine::scrollCamera(int directions)
{
  int x = gfxController.camera.x;
  int y = gfxController.camera.y;
  if (directions & input::LEFT)
    x--;
  if (directions & input::RIGHT)
    x++;
  if (directions & input::DOWN)
    y++;
  if (directions & input::UP)
    y--;
  if (mapController.isPassable({zLevel, x, y}))
  {
    if (tileSize > 8)
      scrollGameSurface(directions);
    else
      SDL_Delay(15);
    gfxController.camera.x = x;
    gfxController.camera.y = y;
  }
}