#include "engine.h"

int main()
{
  GameEngine engine;
  engine.tileSize = 64;
  engine.init();
  engine.run();
  return 0;
}
