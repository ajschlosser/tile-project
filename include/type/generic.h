#ifndef GAME_GENERIC_TYPE_H
#define GAME_GENERIC_TYPE_H

#include "sprite.h"
#include <map>
#include <string>

struct GenericType
{
  std::string name;
  bool impassable;
  float multiplier;
  bool clusters;
  std::map<int, std::map<int, Sprite*>> animationMap;
  int animationSpeed;
  int maxFrames(int direction = 0x02) { return animationMap[direction].size(); }
  Sprite* getFrame(int n, int direction = 0x02) { return animationMap[direction][n]; }
  bool isAnimated() { return animationSpeed > 0; }
  float getMultiplier() { if (multiplier > 0) return multiplier; else return 1; }
  GenericType() {};
  GenericType(
    std::string name,
    bool impassable,
    float multiplier,
    bool clusters,
    std::map<int, std::map<int, Sprite*>> animationMap,
    int animationSpeed
  )
  {
    this->name = name;
    this->impassable = impassable;
    this->multiplier = multiplier;
    this->clusters = clusters;
    this->animationMap = animationMap;
    this->animationSpeed = animationSpeed;
  }
};


#endif