#ifndef GAME_EVENTS_H
#define GAME_EVENTS_H

#include "objects.h"

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <tuple>

namespace events
{

  enum e
  {
    MOVE       = 0X01
  };
  template <class T>
  struct eventData
  {
    std::shared_ptr<T> trigger;
    int x;
    int y;
    int z;
    std::map<std::string, std::string> meta;
    eventData () {}
    eventData (eventData* e)
    {
      trigger = e->trigger;
      x = e->x;
      y = e->y;
      z = e->z;
      meta = e->meta;
    }
  };
  
  template <typename T> void push(int, std::shared_ptr<eventData<T>>);
  template <typename T> std::tuple<int, std::shared_ptr<eventData<T>>> pop();
}

template <typename T> std::queue<std::tuple<int, std::shared_ptr<events::eventData<T>>>> _q;

template <typename T> void events::push(int e, std::shared_ptr<events::eventData<T>> d)
{
  if (d.get()->trigger.get()->type == tileObject::MOB)
    _q<T>.push(std::make_tuple( e, d ));
}

#endif