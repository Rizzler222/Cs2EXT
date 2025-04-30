#include "Aimbot.h"
#include "Entity.h"
#include <algorithm>
#include <cfloat>
#include <iostream>
#include <Windows.h>

Aimbot::Aimbot(Memory& memory) :
    mem(memory),
    currentTargetAddress(0),
    isTargetLocked(false)
{
}

void Aimbot::ResetTarget() {
    currentTargetAddress = 0;
    isTargetLocked = false;
}

Entity Aimbot::GetLocalPlayer() {
    Entity localPlayer;
    uintptr_t localPlayerController = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwLocalPlayerController);
    localPlayer.pawnAddress = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwLocalPlayerPawn);
    if (localPlayer.pawnAddress) {
        localPlayer.team = mem.Read<int>(localPlayer.pawnAddress + Offsets::m_iTeamNum);
        localPlayer.origin = mem.Read<Vec3>(localPlayer.pawnAddress + Offsets::m_vOldOrigin);
        localPlayer.shotsFired = mem.Read<int>(localPlayer.pawnAddress + Offsets::m_iShotsFired);
        localPlayer.velocity = mem.Read<Vec3>(localPlayer.pawnAddress + Offsets::m_vecVelocity);
    }
    return localPlayer;
}


Vec3 Aimbot::GetBonePosition(uintptr_t entityPawn, int bone) {
    uintptr_t sceneNode = mem.Read<uintptr_t>(entityPawn + Offsets::m_pGameSceneNode);
    uintptr_t boneMatrix = mem.Read<uintptr_t>(sceneNode + Offsets::m_modelState + 0x80);
    return mem.Read<Vec3>(boneMatrix + bone * Config::BONE_MATRIX_SIZE);
}

std::vector<Entity> Aimbot::GetEntities(const Entity& localPlayer) {
    std::vector<Entity> entities;
    uintptr_t entityList = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwEntityList);
    uintptr_t listEntry = mem.Read<uintptr_t>(entityList + 0x10);

    for (int i = 0; i < 64; i++) {
        if (!listEntry) continue;

        uintptr_t controller = mem.Read<uintptr_t>(listEntry + i * 0x78);
        if (!controller) continue;

        int pawnHandle = mem.Read<int>(controller + Offsets::m_hPlayerPawn);
        if (!pawnHandle) continue;

        uintptr_t listEntry2 = mem.Read<uintptr_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
        uintptr_t currentPawn = mem.Read<uintptr_t>(listEntry2 + 0x78 * (pawnHandle & 0x1FF));

        if (currentPawn == localPlayer.pawnAddress) continue;

        Entity entity;
        entity.pawnAddress = currentPawn;
        entity.name = mem.ReadString(controller + Offsets::m_iszPlayerName, 128); // Read name from controller
        entity.health = mem.Read<int>(currentPawn + Offsets::m_iHealth);
        entity.team = mem.Read<int>(currentPawn + Offsets::m_iTeamNum);

        if (entity.health <= 0 || entity.team == localPlayer.team) continue;

        entity.origin = mem.Read<Vec3>(currentPawn + Offsets::m_vOldOrigin);

        for (const auto& bonePair : Config::BONE_IDS) {
            entity.bones[bonePair.second] = GetBonePosition(currentPawn, bonePair.second);
        }

        entity.head = entity.bones[6];
        entity.velocity = mem.Read<Vec3>(currentPawn + Offsets::m_vecVelocity);
        entity.distance = entity.origin.Distance(localPlayer.origin);

        entities.push_back(entity);
    }

    return entities;
}

Entity* Aimbot::GetBestTarget(std::vector<Entity>& entities, const Vec3& playerView) {
    if (entities.empty()) return nullptr;
    if (isTargetLocked && currentTargetAddress != 0) {
        auto it = std::find_if(entities.begin(), entities.end(),
            [this](const Entity& entity) { return entity.pawnAddress == currentTargetAddress; });
        if (it != entities.end() && it->health > 0) {
            return &(*it);
        }
        ResetTarget();
    }
    if (!isTargetLocked) {
        Entity* bestTarget = nullptr;
        float bestScreenDistance = FLT_MAX;
        ViewMatrix viewMatrix = mem.Read<ViewMatrix>(mem.GetModuleBase() + Offsets::dwViewMatrix);
        Vec2 screenCenter(Config::SCREEN_WIDTH / 2.0f, Config::SCREEN_HEIGHT / 2.0f);

        // Calculate FOV radius in pixels - this makes FOV consistent with the circle
        float fovRadians = (Config::AIM_FOV / 2.0f) * (3.14159265358979323846f / 180.0f);
        float distanceToPlane = Config::SCREEN_HEIGHT / 2.0f / tan(70.0f * 0.5f * (3.14159265358979323846f / 180.0f));
        float fovRadiusInPixels = tan(fovRadians) * distanceToPlane;

        for (auto& entity : entities) {
            // Predict target position based on velocity
            Vec3 relativeVelocity = entity.velocity - playerView; // playerView is the local player's velocity
            Vec3 predictedPosition = entity.bones.at(Config::SELECTED_BONE) + (relativeVelocity * Config::PREDICTION_TIME);
            Vec2 targetScreenPos = WorldToScreen(viewMatrix, predictedPosition, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);
            float screenDistance = screenCenter.Distance(targetScreenPos);

            // Check if the target is within the FOV using pixel distance
            if (screenDistance <= fovRadiusInPixels) {
                if (screenDistance < bestScreenDistance) {
                    bestScreenDistance = screenDistance;
                    bestTarget = &entity;
                }
            }
        }
        if (bestTarget) {
            currentTargetAddress = bestTarget->pawnAddress;
            isTargetLocked = true;
        }
        return bestTarget;
    }
    return nullptr;
}


void Aimbot::AimAtTarget(const Entity* target, const Entity& localPlayer, const Vec3& playerView, bool useSmoothing) {
    if (!target) return;

    ViewMatrix viewMatrix = mem.Read<ViewMatrix>(mem.GetModuleBase() + Offsets::dwViewMatrix);

    // Predict target position based on velocity
    Vec3 relativeVelocity = target->velocity - localPlayer.velocity;
    Vec3 predictedPosition = target->bones.at(Config::SELECTED_BONE) + (relativeVelocity * Config::PREDICTION_TIME);
    Vec2 targetScreenPos = WorldToScreen(viewMatrix, predictedPosition, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT);

    Vec2 screenCenter(Config::SCREEN_WIDTH / 2.0f, Config::SCREEN_HEIGHT / 2.0f);

    float deltaX = targetScreenPos.x - screenCenter.x;
    float deltaY = targetScreenPos.y - screenCenter.y;

    int moveX, moveY;

    if (useSmoothing) {
        float smoothFactor = Config::SMOOTHING;
        moveX = static_cast<int>(deltaX / smoothFactor);
        moveY = static_cast<int>(deltaY / smoothFactor);

        // Apply minimum movement threshold to prevent micro-adjustments
        float minMovementThreshold = 1.0f;
        if (std::abs(moveX) < minMovementThreshold && std::abs(deltaX) > 0) {
            moveX = (deltaX > 0) ? 1 : -1;
        }
        if (std::abs(moveY) < minMovementThreshold && std::abs(deltaY) > 0) {
            moveY = (deltaY > 0) ? 1 : -1;
        }
    }
    else {
        // Use precise rounding for maximum efficiency when smoothing is off
        moveX = static_cast<int>(std::round(deltaX));
        moveY = static_cast<int>(std::round(deltaY));
    }

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    input.mi.dx = moveX;
    input.mi.dy = moveY;
    SendInput(1, &input, sizeof(INPUT));
}