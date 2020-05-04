#ifndef GAME_GENERIC_TYPE_H
#define GAME_GENERIC_TYPE_H

#include "../sprite.h"
#include "../objects.h"
#include <map>
#include <string>

struct GenericType
{
  Sprite* sprite;
  std::string name;
  bool impassable;
  float multiplier;
  bool clusters;
  std::map<int, std::map<int, Sprite*>> animationMap;
  int animationSpeed;
  int maxFrames(int direction = tileObject::DOWN) { return animationMap[direction].size(); }
  Sprite* getFrame(int n, int direction = tileObject::DOWN) { return animationMap[direction][n]; }
  bool isAnimated() { return animationSpeed > 0; }
  float getMultiplier() { if (multiplier > 0) return multiplier; else return 1; }
};


#endif