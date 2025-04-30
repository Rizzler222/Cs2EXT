#pragma once
#include "Memory.h"
#include "Entity.h"
#include "Vector.h"
#include "Offsets.h"
#include "Config.h"
#include <vector>

class Aimbot {
private:
    Memory& mem;
    uintptr_t currentTargetAddress;
    bool isTargetLocked;

    Vec3 GetBonePosition(uintptr_t entityPawn, int bone);

public:
    Aimbot(Memory& memory);
    void ResetTarget();
    Entity GetLocalPlayer();
    std::vector<Entity> GetEntities(const Entity& localPlayer);
    Entity* GetBestTarget(std::vector<Entity>& entities, const Vec3& playerView);
    void AimAtTarget(const Entity* target, const Entity& localPlayer, const Vec3& playerView, bool useSmoothing);
};