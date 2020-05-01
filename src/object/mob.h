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
    simulators.push_back(std::make_shared<simulated::Simulator<MobObject>>(this,[](MobObject* m){

      int n = std::rand() % 100;

      

    }));
  }
};

#endif