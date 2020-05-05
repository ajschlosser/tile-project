#ifndef GAME_MOB_OBJECT_H
#define GAME_MOB_OBJECT_H

#include "simulated.h"

struct MobObject : SimulatedObject
{
  std::string id;
  int speed;
  MobType* mobType;
  std::map<std::string, Timer> mobTimers;
  std::vector<std::shared_ptr<simulated::Simulator<MobObject>>> simulators;
  MobObject (int x, int y, int z, MobType* m, BiomeType* b)
  {
    type = tileObject::MOB;
    id = uuid::generate_uuid_v4();
    this->initSimulation();
    this->x = x;
    this->y = y;
    this->z = z;
    mobType = m;
    biomeType = b;
    speed = std::rand() % 1500 + 1000;
    Timer t;
    t.start();
    mobTimers["movement"] = t;
  }
};

#endif