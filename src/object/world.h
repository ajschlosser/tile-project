#ifndef GAME_WORLD_OBJECT_H
#define GAME_WORLD_OBJECT_H

#include "tile.h"

struct WorldObject : Tile
{
  ObjectType* objectType;
  WorldObject() { type = tileObject::WORLD; }
  WorldObject(int x, int y, ObjectType* o, BiomeType* b)
  {
    type = tileObject::WORLD;
    this->x = x;
    this->y = y;
    objectType = o;
    biomeType = b;
    sprite = o->sprite;
  }
};

#endif