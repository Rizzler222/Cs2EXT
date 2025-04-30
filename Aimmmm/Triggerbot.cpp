#include "Triggerbot.h"
#include <iostream>
#include <Windows.h>

Triggerbot::Triggerbot(Memory& memory) :
    mem(memory),
    isEnabled(false),
    lastCrosshairID(0),
    delay(0) // Initialize delay to 0 ms
{
    clientBase = mem.GetModuleBase();
}

void Triggerbot::SetDelay(int delayMs) {
    delay = delayMs;
}

int Triggerbot::GetDelay() const {
    return delay;
}

void Triggerbot::SetEnabled(bool enabled) {
    isEnabled = enabled;
    std::cout << "Triggerbot " << (isEnabled ? "Enabled" : "Disabled") << std::endl;
}

bool Triggerbot::IsEnabled() const {
    return isEnabled;
}

void Triggerbot::Update(const Entity& localPlayer) {
    if (!isEnabled || !localPlayer.pawnAddress) {
        return;
    }

    int crosshairID = mem.Read<int>(localPlayer.pawnAddress + 0x1458);
    std::cout << "Crosshair ID: " << crosshairID << std::endl;

    if (crosshairID == -1) {
        std::cout << "Crosshair not over any entity. Skipping update." << std::endl;
        return;
    }

    uintptr_t entityList = mem.Read<uintptr_t>(clientBase + Offsets::dwEntityList);
    if (!entityList) {
        std::cout << "Failed to read entity list." << std::endl;
        return;
    }

    uintptr_t listEntry = mem.Read<uintptr_t>(entityList + 0x8 * ((crosshairID & 0x7FFF) >> 9) + 0x10);
    if (!listEntry) {
        std::cout << "Failed to read list entry for crosshair ID: " << crosshairID << std::endl;
        return;
    }

    uintptr_t entity = mem.Read<uintptr_t>(listEntry + 0x78 * (crosshairID & 0x1FF));
    if (!entity) {
        std::cout << "Failed to read entity address for crosshair ID: " << crosshairID << std::endl;
        return;
    }

    int entityTeam = mem.Read<int>(entity + Offsets::m_iTeamNum);
    int entityHealth = mem.Read<int>(entity + Offsets::m_iHealth);

    std::cout << "Entity Team: " << entityTeam << ", Local Team: " << localPlayer.team << std::endl;
    std::cout << "Entity Health: " << entityHealth << std::endl;

    if (entityHealth > 0 && entityTeam != localPlayer.team) {
        std::cout << "Triggering shot at enemy." << std::endl;

        // Add delay before shooting
        Sleep(delay);

        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));

        Sleep(0);

        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    else {
        std::cout << "Entity is not a valid target (health or team check failed)." << std::endl;
    }
}