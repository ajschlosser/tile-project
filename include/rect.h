#ifndef GAME_RECT_H
#define GAME_RECT_H

#include "SDL2/SDL.h"
#include <cmath>
#include <functional>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

struct Rect
{
  int x1;
  int y1;
  int x2;
  int y2;
  std::vector<Rect> rects;
  Rect () {}
  Rect (int a, int b, int c, int d) { x1 = a; y1 = b; x2 = c; y2 = d; }
  void set(std::tuple<int, int, int, int> data) { auto [_x1, _y1, _x2, _y2] = data; x1 = _x1; y1 =_y1; x2 =_x2; y2 = _y2; }
  std::tuple<int, int, int, int> get(){ return std::make_tuple(x1, y1, x2, y2 ); };
  SDL_Rect* getSDL_Rect () { auto r = new SDL_Rect(); r->x = x1; r->y = y1; r->w = x2; r->h = y2; return r; }
  int getWidth () { return std::abs(x1) + std::abs(x2); }
  int getHeight () { return std::abs(y1) + std::abs(y2); }
  std::pair<int, int> getDimensions () { return { getWidth(), getHeight() }; }
  std::tuple<int, int> getMid (){ return std::make_tuple(std::floor((x1 + x2)/2), std::floor((y1 + y2)/2)); }
  std::vector<Rect>* getRects(bool shuffle = false) // TODO: Don't clear every time, create different method
  {
    int small_w = 25;
    int small_h = 25;
    int w = getWidth();
    int h = getHeight();
    auto result_w = std::div(w, small_w);
    auto result_h = std::div(h, small_h);
    rects.clear();
    for (auto x = x1; x <= x2; x += result_w.quot)
      for (auto y = y1; y <= y2; y += result_h.quot)
      {
        Rect r { x, y, x + result_w.quot, y + result_h.quot };
        rects.push_back(r);
      }
    if (shuffle == true)
    {
      unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::default_random_engine rng(seed);
      std::shuffle(rects.begin(), rects.end(), rng);
    }
    return &rects;
  }
  std::vector<Rect>* getShuffledRects() { return getRects(true); }
  void multiprocess(std::function<void(int, int)> f, Rect* r = NULL, int fuzz = 1)
  {
    if (r == NULL)
      r->set({ x1, y1, x2, y2 });
    for (auto i = x1; i < x2; i += 1 + std::rand() % fuzz)
      for (auto j = y1; j < y2; j += 1 + std::rand() % fuzz)
      {
        std::thread t([&f, i, j]() { f(i, j); });
        t.join();
      }
  }
};

#endif