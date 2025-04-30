#pragma once
#include "Memory.h"
#include "Entity.h"
#include "Offsets.h"

class Triggerbot {
private:
    Memory& mem;
    uintptr_t clientBase;
    bool isEnabled;
    int lastCrosshairID;
    int delay; // Add this line to declare the delay variable

public:
    Triggerbot(Memory& memory);
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    void SetDelay(int delayMs); // Setter for delay
    int GetDelay() const;      // Getter for delay
    void Update(const Entity& localPlayer);
};