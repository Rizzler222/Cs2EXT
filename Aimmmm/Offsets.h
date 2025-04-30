#pragma once

namespace Offsets {
    constexpr auto dwViewAngles = 0x1AACA70;
    constexpr auto dwLocalPlayerPawn = 0x188AF20;
    constexpr auto dwEntityList = 0x1A36A00;
    constexpr auto m_hPlayerPawn = 0x80C;
    constexpr auto m_iHealth = 0x344;
    constexpr auto m_vOldOrigin = 0x1324;
    constexpr auto m_iTeamNum = 0x3E3;
    constexpr auto m_vecViewOffset = 0xCB0;
    constexpr auto m_lifeState = 0x348;
    constexpr auto m_modelState = 0x170;
    constexpr auto m_pGameSceneNode = 0x328;
    constexpr auto dwViewMatrix = 0x1AA27F0;
    constexpr auto m_vecVelocity = 0x400;
    constexpr auto m_aimPunchAngle = 0x1584;
    constexpr auto m_iShotsFired = 0x23FC;
    constexpr uintptr_t m_iszPlayerName = 0x660; // char[128]
    constexpr auto dwLocalPlayerController = 0x1A88080;
    constexpr uintptr_t m_flFlashMaxAlpha = 0x1408; // float32
    constexpr uintptr_t m_hOwnerEntity = 0x440;
    constexpr uintptr_t m_flLastSmokeOverlayAlpha = 0x14B0;
    constexpr uintptr_t m_bIsScoped = 0x23E8;
    constexpr uintptr_t m_pCameraServices = 0x11E0;
    constexpr uintptr_t m_iFOV = 0x210;
    constexpr uintptr_t m_iIDEntIndex = 0x1458;
    constexpr uintptr_t m_iObserverMode = 0x40;
    constexpr uintptr_t m_hObserverTarget = 0x44; 
    constexpr uintptr_t m_pObserverServices = 0x11C0;
    constexpr uintptr_t m_hPawn = 0x62C;
}

namespace C_SmokeGrenadeProjectile {
    constexpr uintptr_t m_vSmokeColor = 0x121C;
    constexpr uintptr_t m_bDidSmokeEffect = 0x1214;
    constexpr uintptr_t m_bSmokeEffectSpawned = 0x1259;
    constexpr uintptr_t m_nVoxelFrameDataSize = 0x1250;
}

enum ObserverMode_t : uint32_t {
    OBS_MODE_NONE = 0,
    OBS_MODE_FIXED = 1,
    OBS_MODE_FIRSTPERSON = 2,
    OBS_MODE_THIRDPERSON = 3,
    OBS_MODE_CHASE = 4,
    OBS_MODE_ROAMING = 5,
    OBS_MODE_DIRECTED = 6
};