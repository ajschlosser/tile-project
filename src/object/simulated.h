#ifndef GAME_SIMULATED_OBJECT_H
#define GAME_SIMULATED_OBJECT_H

#include "tile.h"

namespace simulated
{
  class Simulator
  {
    private:
      
    public:
      Simulator () {}
  };
}

struct SimulatedObject : Tile
{
  std::map<std::string, Timer*> objectTimers;
  bool dead;
  int direction;
  SimulatedObject () : dead(false), direction(tileObject::DOWN) {}
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