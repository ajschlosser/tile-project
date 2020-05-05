#ifndef GAME_SIMULATED_OBJECT_H
#define GAME_SIMULATED_OBJECT_H

#include "tile.h"
#include <memory>
#include <tuple>



namespace simulated
{
  enum actions
  {
    MOVE      = 0x01,
    DIE       = 0x02,
    DELETE    = 0x04
  };
  template <class T>
  class Simulator
  {
    private:
      std::string _id;
      Timer _timer;
      int _frequency;
      int _variation;
      std::function<void()> _fn;
    public:
      Simulator () {}
      Simulator (std::function<void()> fn)
      {
        _fn = fn;
        _id = uuid::generate_uuid_v4();
        _timer.start();
        _frequency = 3000 + std::rand() % 1000;
      }
      void simulate ()
      {
        auto elapsed = _timer.elapsed();
        if (elapsed > _frequency)
        {
          _fn();
          _timer.reset();
        }
      }
  };
}

struct SimulatedObject : Tile
{
  std::map<std::string, Timer*> objectTimers;
  bool dead;
  int orders;
  int status;
  SimulatedObject () : dead(false) { type = tileObject::SIMULATED; }
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