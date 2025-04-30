#pragma once
#include <vector>
#include <map>
#include <string>
#include <Windows.h>


constexpr int MOUSE_LEFT = VK_LBUTTON;
constexpr int MOUSE_RIGHT = VK_RBUTTON;
constexpr int MOUSE_MIDDLE = VK_MBUTTON;
constexpr int MOUSE_X1 = VK_XBUTTON1;
constexpr int MOUSE_X2 = VK_XBUTTON2;

namespace Config {
    constexpr int SCREEN_WIDTH = 1920;
    constexpr int SCREEN_HEIGHT = 1080;
    extern float SMOOTHING;
    extern int SELECTED_BONE;
    extern int AIMKEY;
    extern float AIM_FOV;              // Declare AIM_FOV as extern
    inline bool SHOW_FOV_CIRCLE = true; // Toggle for FOV circle
    constexpr float PREDICTION_TIME = 0.050f; // Adjust based on testing
    extern bool FLICK_ENABLED;
    extern int FLICKKEY;
    extern float OVERSHOOT_PERCENT;
    extern float RETURN_PERCENT;
    inline bool NO_FLASH = false;
    inline bool NO_SMOKE = false;
    inline int VFOV = 90; // Default FOV value
    inline int ScopedFOV = 60; // FOV when scoped
    inline bool SHOW_SPECTATORS = true;
    inline bool SPECTATORS_ONLY_LOCAL = true;
    inline bool VFOVChangerEnabled = false; // Enable/disable FOV changer


    struct BoneHotkey {
        int boneId;
        int hotkey;
        std::string displayName;
    };

    extern std::vector<BoneHotkey> BONE_HOTKEYS;

    extern const std::map<std::string, int> BONE_IDS;

    constexpr int BONE_MATRIX_SIZE = 32;
    constexpr float BULLET_SPEED = 8000.0f;
}