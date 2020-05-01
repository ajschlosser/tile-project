#ifndef GAME_SIMULATED_OBJECT_H
#define GAME_SIMULATED_OBJECT_H

#include "tile.h"
#include <memory>
#include <tuple>

namespace simulated
{
  template <class T>
  class Simulator
  {
    private:
      std::string _id;
      Timer _timer;
      int _frequency;
      int _variation;
      T* _simulatedObject;
      std::function<void(T*)> _fn;
    public:
      Simulator () {}
      Simulator (T* o, std::function<void(T*)> fn)
      {
        _simulatedObject = o;
        _fn = fn;
        _id = uuid::generate_uuid_v4();
        _timer.start();
        _frequency = 3000 + std::rand() * 3000;
      }
      std::tuple<bool, T*> simulate ()
      {
        auto elapsed = _timer.elapsed();
        if (elapsed > _frequency)
        {
          _fn(_simulatedObject);
          _timer.reset();
          return std::make_tuple(true, _simulatedObject);
        }
        return std::make_tuple(false, _simulatedObject);
      }
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