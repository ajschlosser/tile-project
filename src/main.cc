#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "json/json.h"
#include "objects.h"
#include "engine.h"

int main()
{
  GameEngine engine;
  engine.tileSize = 64;
  engine.init();
  engine.run();
  return 0;
}
