#ifndef GAME_ENGINE_EVENTS_H
#define GAME_ENGINE_EVENTS_H

#include "engine.h"

struct controller::EventsController
{
  GameEngine* e;
  EventsController () {}
  EventsController (GameEngine* e)
  {
    this->e = e;
  }
  void handleEvents();
};

#endif