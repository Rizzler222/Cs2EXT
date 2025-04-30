#pragma once
#include "Vector.h"
#include <map>
#include <string>

struct Entity {
    uintptr_t pawnAddress;
    Vec3 origin;
    Vec3 head;
    std::map<int, Vec3> bones;
    int team;
    int health;
    float distance;
    Vec3 velocity;
    Vec3 aimPunch;
    int shotsFired;
    std::string name; // Add this lin

    Entity();
};