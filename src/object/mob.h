#ifndef GAME_MOB_OBJECT_H
#define GAME_MOB_OBJECT_H

#include "../uuid.h"
#include "simulated.h"

struct MobObject : SimulatedObject
{
  std::string id;
  int speed;
  MobType* mobType;
  std::map<std::string, Timer> mobTimers;
  MobObject (int x, int y, MobType* m, BiomeType* b)
  {
    id = uuid::generate_uuid_v4();
    this->initSimulation();
    this->x = x;
    this->y = y;
    mobType = m;
    biomeType = b;
    sprite = m->sprite;
    speed = std::rand() % 1500 + 1000;
    Timer t;
    t.start();
    mobTimers["movement"] = t;
  }
};

#endif