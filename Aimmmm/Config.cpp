#include "Config.h"

namespace Config {
    float SMOOTHING = 1.0f;
    int SELECTED_BONE = 6;
    int AIMKEY = VK_SHIFT;
    float AIM_FOV = 90.0f; // Define AIM_FOV
    bool FLICK_ENABLED = true;
    int FLICKKEY = VK_XBUTTON2; // Mouse4 by default
    float OVERSHOOT_PERCENT = 0.2f;
    float RETURN_PERCENT = 0.85f;

    std::vector<BoneHotkey> BONE_HOTKEYS = {
        {6, 0x31, "Head"},
        {13, 0x32, "Right Shoulder"},
        {8, 0x33, "Left Shoulder"},
    };

    const std::map<std::string, int> BONE_IDS = {
        {"Head", 6},
        {"Neck", 5},
        {"Spine", 4},
        {"Spine1", 2},
        {"Hip", 0},
        {"Left Shoulder", 8},
        {"Left Arm", 9},
        {"Left Hand", 10},
        {"Right Shoulder", 13},
        {"Right Arm", 14},
        {"Right Hand", 15},
        {"Left Hip", 22},
        {"Left Knee", 23},
        {"Left Feet", 24},
        {"Right Hip", 25},
        {"Right Knee", 26},
        {"Right Feet", 27}
    };
}