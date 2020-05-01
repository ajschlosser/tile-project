#ifndef GAME_TIMER_H
#define GAME_TIMER_H

#include "SDL2/SDL.h"

struct Timer {
  int last;
  int current;
  bool paused;
  bool started;
  void start()
  {
    started = true;
    paused = false;
    current = 0;
    last = SDL_GetTicks();
  }
  void stop()
  {
    started = false;
    paused = false;
    last = 0;
    current = 0;
  }
  void reset() {
    stop();
    start();
  }
  void pause()
  {
    if (started && !paused)
    {
      paused = true;
      current = SDL_GetTicks() - last;
      last = 0;
    }
  }
  void unpause()
  {
    if (started && paused) {
      paused = false;
      last = SDL_GetTicks() - current;
      current = 0;
    }
  }
  int elapsed()
  {
    auto time = 0;
    if (started)
    {
      if (paused)
      {
        time = current;
      }
      else
      {
        time = SDL_GetTicks() - last;
      }
    }
    return time;
  }
  Timer () : last(0), current(0), paused(false), started(false) {}
};

#endif