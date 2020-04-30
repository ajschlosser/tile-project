#ifndef GAME_SIMULATED_OBJECT_H
#define GAME_SIMULATED_OBJECT_H

#include "tile.h"

struct SimulatedObject : Tile
{
  std::map<std::string, Timer*> objectTimers;
  bool dead;
  SimulatedObject () : dead(false) {}
  void kill()
  {
    dead = true;
  }
  void initSimulation()
  {
    Timer t;
    t.start();
    objectTimers["lifetime"] = &t;
  }
};

#endif