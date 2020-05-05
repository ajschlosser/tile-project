#ifndef GAME_MAP_MOB_H
#define GAME_MAP_MOB_H

#include "../map.h"

using namespace map;

std::shared_mutex mobMtx;

std::vector<std::shared_ptr<MobObject>>::iterator
MapController::moveMob (std::string id, std::tuple<int, int, int> origin, std::tuple<int, int, int> destination)
{
  auto [z1, x1, y1] = origin;
  auto [z2, x2, y2] = destination;
  if (!isPassable(destination))
    return mobMap[z1][{x1, y1}].begin();
  std::unique_lock lock(mobMtx);
  auto it = mobMap[z1][{x1, y1}].begin();
  while (it != mobMap[z1][{x1, y1}].end())
  {
    if (it->get()->id == id)
    {
      it->get()->setPosition({ z2, x2, y2 });
      mobMap[z2][{x2, y2}].push_back((*it));
      it = mobMap[z1][{x1, y1}].erase(it);
      return it;
    }
    else
    {
      ++it;
    }
  }
  return it;
}

std::vector<std::shared_ptr<MobObject>>::iterator
MapController::moveMob (std::vector<std::shared_ptr<MobObject>>::iterator mobIt, std::tuple<int, int, int> coords, int directions)
{
  auto mob = mobIt->get();
  auto [z1, x1, y1] = coords;
  auto [z2, x2, y2] = coords;
  if (directions & tileObject::LEFT)
  {
    if (mob->direction == tileObject::LEFT)
    {
      x2--;
    }
    else
    {
      mob->direction = tileObject::LEFT;
      return ++mobIt;
    }
  }
  if (directions & tileObject::RIGHT)
  {
    if (mob->direction == tileObject::RIGHT)
    {
      x2++;
    }
    else
    {
      mob->direction = tileObject::RIGHT;
      return ++mobIt;
    }
  }
  if (directions & tileObject::UP)
  {
    if (mob->direction == tileObject::UP)
    {
      y2--;
    }
    else
    {
      mob->direction = tileObject::UP;
      return ++mobIt;
    }
  }
  if (directions & tileObject::DOWN)
  {
    if (mob->direction == tileObject::DOWN)
    {
      y2++;
    }
    else
    {
      mob->direction = tileObject::DOWN;
      return ++mobIt;
    }
  }
  SDL_Point offset = {0, 0};
  if (mob->direction == tileObject::RIGHT)
    offset.x -= cfg->tileSize;
  if (mob->direction == tileObject::DOWN)
    offset.y -= cfg->tileSize;
  if (mob->direction == tileObject::UP)
    offset.y += cfg->tileSize;
  if (mob->direction == tileObject::LEFT)
    offset.x += cfg->tileSize;
  mob->relativeX = offset.x;
  mob->relativeY = offset.y;
  return moveMob(mob->id, {z1, x1, y1}, {z2, x2, y2});
}

#endif