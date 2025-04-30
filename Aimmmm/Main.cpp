            #define NOMINMAX
            #include <Windows.h>
            #include <vector>
            #include <thread>
            #include <algorithm>
            #include <cmath>
            #include <iostream>
            #include <string>
            #include <map>
            #include <unordered_map>

            #include "Memory.h"
            #include "Vector.h"
            #include "Config.h"
            #include "Offsets.h"
            #include "Entity.h"
            #include "Aimbot.h"
            #include "Triggerbot.h"

            #include "imgui.h"
            #include "imgui_impl_win32.h"
            #include "imgui_impl_dx11.h"
            #include <d3d11.h>
            #include <tchar.h>



            void DrawSpectatorList(Memory& mem, const Entity& localPlayer, int screenWidth) {
                if (!Config::SHOW_SPECTATORS) return;

                uintptr_t localController = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwLocalPlayerController);
                uintptr_t entityList = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwEntityList);

                std::vector<std::string> spectators;

                for (int i = 0; i < 64; i++) {
                    uintptr_t listEntry = mem.Read<uintptr_t>(entityList + 8 * ((i & 0x7FFF) >> 9) + 16);
                    if (!listEntry) continue;

                    uintptr_t controller = mem.Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
                    if (!controller || controller == localController) continue;

                    uint32_t pawnHandle = mem.Read<uint32_t>(controller + Offsets::m_hPawn);
                    if (!pawnHandle) continue;

                    uintptr_t listEntry2 = mem.Read<uintptr_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
                    uintptr_t pawn = mem.Read<uintptr_t>(listEntry2 + 0x78 * (pawnHandle & 0x1FF));
                    if (!pawn) continue;

                    uintptr_t observerServices = mem.Read<uintptr_t>(pawn + Offsets::m_pObserverServices);
                    if (!observerServices) continue;

                    uint32_t targetHandle = mem.Read<uint32_t>(observerServices + Offsets::m_hObserverTarget);
                    int observerMode = mem.Read<int>(observerServices + Offsets::m_iObserverMode);

                    if (!targetHandle) continue;

                    uintptr_t targetListEntry = mem.Read<uintptr_t>(entityList + 0x8 * ((targetHandle & 0x7FFF) >> 9) + 0x10);
                    uintptr_t targetPawn = mem.Read<uintptr_t>(targetListEntry + 0x78 * (targetHandle & 0x1FF));
                    if (!targetPawn) continue;

                    if (Config::SPECTATORS_ONLY_LOCAL && targetPawn != localPlayer.pawnAddress) continue;

                    // Only show First/Third person spectators
                    if (observerMode != 2 && observerMode != 3) continue;

                    std::string specName = mem.ReadString(controller + Offsets::m_iszPlayerName, 128);
                    spectators.push_back(specName);
                }

                if (!spectators.empty()) {
                    // Position and style configuration
                    const int startX = 10; // X position for the spectator list
                    const int startY = 320; // Y position for the spectator list
                    const ImU32 textColor = IM_COL32(255, 255, 255, 255); // White text color
                    const ImU32 backgroundColor = IM_COL32(128, 128, 128, 100); // Fully opaque black background
                    const ImU32 headerColor = IM_COL32(255, 0, 0, 255); // Red header text color

                    // Draw the header
                    const std::string headerText = "Spectators:";
                    ImVec2 headerTextSize = ImGui::CalcTextSize(headerText.c_str());

                    // Draw black background for the header
                    ImGui::GetBackgroundDrawList()->AddRectFilled(
                        ImVec2(startX, startY),
                        ImVec2(startX + headerTextSize.x + 10, startY + headerTextSize.y + 5),
                        backgroundColor
                    );

                    // Draw the header text
                    ImGui::GetBackgroundDrawList()->AddText(
                        ImVec2(startX + 5, startY + 2),
                        headerColor,
                        headerText.c_str()
                    );

                    // Draw each spectator entry
                    int yOffset = headerTextSize.y + 10; // Start below the header
                    for (const auto& spec : spectators) {
                        ImVec2 textSize = ImGui::CalcTextSize(spec.c_str());

                        // Draw black background for the text
                        ImGui::GetBackgroundDrawList()->AddRectFilled(
                            ImVec2(startX, startY + yOffset),
                            ImVec2(startX + textSize.x + 10, startY + yOffset + textSize.y + 5),
                            backgroundColor
                        );

                        // Draw the spectator text
                        ImGui::GetBackgroundDrawList()->AddText(
                            ImVec2(startX + 5, startY + yOffset + 2),
                            textColor,
                            spec.c_str()
                        );

                        // Move to the next line
                        yOffset += textSize.y + 5; // Adjust spacing between lines
                    }
                }
            }

            // Global DirectX variables
            ID3D11Device* g_pd3dDevice = nullptr;
            ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
            IDXGISwapChain* g_pSwapChain = nullptr;
            ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

            // Forward declarations
            LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
            extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


            void HandleFOVChanger(Memory& mem, const Entity& localPlayer) {
                if (!Config::VFOVChangerEnabled || localPlayer.pawnAddress == 0) return;

                // Check if the player is scoped
                bool isScoped = mem.Read<bool>(localPlayer.pawnAddress + Offsets::m_bIsScoped);

                // If the player is scoped, do not modify the FOV
                if (isScoped) {
                    return;
                }

                // Read camera services pointer
                uintptr_t cameraServicesPtr = mem.Read<uintptr_t>(localPlayer.pawnAddress + Offsets::m_pCameraServices);

                if (cameraServicesPtr != 0) {
                    // Write the new FOV value to memory (only when not scoped)
                    mem.Write<int>(cameraServicesPtr + Offsets::m_iFOV, Config::VFOV);
                }
            }

            void HandleNoSmoke(Memory& mem, const Entity& localPlayer) {
                if (!Config::NO_SMOKE || localPlayer.pawnAddress == 0) return;

                uintptr_t entityList = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwEntityList);

                for (int i = 0; i < 1024; i++) {
                    uintptr_t listEntry = mem.Read<uintptr_t>(entityList + 0x8 * ((i & 0x7FFF) >> 9) + 0x10);
                    if (!listEntry) continue;

                    uintptr_t entity = mem.Read<uintptr_t>(listEntry + 0x78 * (i & 0x1FF));
                    if (!entity) continue;

                    // Read class name
                    char className[32] = { 0 };
                    uintptr_t entityIdentity = mem.Read<uintptr_t>(entity + 0x10);
                    if (entityIdentity) {
                        uintptr_t designerNamePtr = mem.Read<uintptr_t>(entityIdentity + 0x20);
                        if (designerNamePtr) {
                            mem.ReadRaw(designerNamePtr, className, sizeof(className) - 1);
                        }
                    }

                    if (strstr(className, "smokegrenade_projectile")) {
                        // Create a Vector4 for RGBA color (alpha = 0 for transparency)
                        Vector4 transparentColor;
                        transparentColor.x = 1.0f; // Red
                        transparentColor.y = 1.0f; // Green
                        transparentColor.z = 1.0f; // Blue
                        transparentColor.w = 0.0f; // Alpha (0 = fully transparent)

                        // Write the transparent color to the smoke entity
                        mem.Write<Vector4>(entity + C_SmokeGrenadeProjectile::m_vSmokeColor, transparentColor);

                        // Prevent smoke effect from being processed
                        mem.Write<bool>(entity + C_SmokeGrenadeProjectile::m_bDidSmokeEffect, true);
                        mem.Write<bool>(entity + C_SmokeGrenadeProjectile::m_bSmokeEffectSpawned, true);
                    }
                }
            }


            // Utility functions
            std::string GetKeyName(int key) {
                switch (key) {
                case VK_LBUTTON: return "Mouse Left";
                case VK_RBUTTON: return "Mouse Right";
                case VK_MBUTTON: return "Mouse Middle";
                case VK_XBUTTON1: return "Mouse X1";
                case VK_XBUTTON2: return "Mouse X2";
                default: {
                    char keyName[64];
                    if (GetKeyNameTextA(MapVirtualKeyA(key, MAPVK_VK_TO_VSC) << 16, keyName, sizeof(keyName))) {
                        return keyName;
                    }
                    return "Unknown";
                }
                }
            }

            void ApplyModernTheme() {
                ImGuiStyle& style = ImGui::GetStyle();
                ImVec4* colors = style.Colors;

                // Modern neon purple theme
                const ImVec4 accentColor = ImVec4(0.58f, 0.27f, 0.96f, 1.00f);
                const ImVec4 accentHover = ImVec4(0.68f, 0.37f, 1.00f, 1.00f);
                const ImVec4 backgroundDark = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
                const ImVec4 backgroundMedium = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
                const ImVec4 textColor = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);

                // Style adjustments
                style.WindowPadding = ImVec2(12, 12);
                style.FramePadding = ImVec2(10, 6);
                style.ItemSpacing = ImVec2(10, 8);
                style.ScrollbarSize = 10;
                style.WindowRounding = 10.0f;
                style.FrameRounding = 6.0f;
                style.PopupRounding = 6.0f;
                style.ScrollbarRounding = 6.0f;
                style.GrabRounding = 4.0f;
                style.TabRounding = 6.0f;
                style.ChildRounding = 8.0f;

                // Color scheme
                colors[ImGuiCol_Text] = textColor;
                colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
                colors[ImGuiCol_WindowBg] = backgroundDark;
                colors[ImGuiCol_ChildBg] = backgroundMedium;
                colors[ImGuiCol_PopupBg] = backgroundMedium;
                colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
                colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
                colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
                colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
                colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
                colors[ImGuiCol_TitleBg] = backgroundMedium;
                colors[ImGuiCol_TitleBgActive] = accentColor;
                colors[ImGuiCol_CheckMark] = accentColor;
                colors[ImGuiCol_SliderGrab] = accentColor;
                colors[ImGuiCol_SliderGrabActive] = accentHover;
                colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
                colors[ImGuiCol_ButtonHovered] = accentColor;
                colors[ImGuiCol_ButtonActive] = accentHover;
                colors[ImGuiCol_Header] = accentColor;
                colors[ImGuiCol_HeaderHovered] = accentHover;
                colors[ImGuiCol_HeaderActive] = accentHover;
                colors[ImGuiCol_SeparatorHovered] = accentColor;
                colors[ImGuiCol_SeparatorActive] = accentHover;
                colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
                colors[ImGuiCol_ResizeGripHovered] = accentColor;
                colors[ImGuiCol_ResizeGripActive] = accentHover;
                colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
                colors[ImGuiCol_TabHovered] = ImVec4(
                    accentColor.x * 0.9f,
                    accentColor.y * 0.9f,
                    accentColor.z * 0.9f,
                    accentColor.w * 0.6f
                );
                colors[ImGuiCol_TabActive] = ImVec4(
                    accentColor.x * 1.1f,
                    accentColor.y * 1.1f,
                    accentColor.z * 1.1f,
                    accentColor.w * 0.8f
                );
            }

            void CreateRenderTarget() {
                ID3D11Texture2D* pBackBuffer = nullptr;
                g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
                if (pBackBuffer) {
                    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
                    pBackBuffer->Release();
                }
            }

            void CleanupRenderTarget() {
                if (g_mainRenderTargetView) {
                    g_mainRenderTargetView->Release();
                    g_mainRenderTargetView = nullptr;
                }
            }

            static bool prevKeyStates[256] = { false }; // Track previous key states

            bool CreateDeviceD3D(HWND hWnd) {
                DXGI_SWAP_CHAIN_DESC sd;
                ZeroMemory(&sd, sizeof(sd));
                sd.BufferCount = 2;
                sd.BufferDesc.Width = 0;
                sd.BufferDesc.Height = 0;
                sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                sd.BufferDesc.RefreshRate.Numerator = 60;
                sd.BufferDesc.RefreshRate.Denominator = 1;
                sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
                sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                sd.OutputWindow = hWnd;
                sd.SampleDesc.Count = 1;
                sd.SampleDesc.Quality = 0;
                sd.Windowed = TRUE;
                sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

                UINT createDeviceFlags = 0;
                D3D_FEATURE_LEVEL featureLevel;
                const D3D_FEATURE_LEVEL featureLevelArray[2] = {
                    D3D_FEATURE_LEVEL_11_0,
                    D3D_FEATURE_LEVEL_10_0,
                };
                if (D3D11CreateDeviceAndSwapChain(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    createDeviceFlags,
                    featureLevelArray,
                    2,
                    D3D11_SDK_VERSION,
                    &sd,
                    &g_pSwapChain,
                    &g_pd3dDevice,
                    &featureLevel,
                    &g_pd3dDeviceContext) != S_OK)
                    return false;

                CreateRenderTarget();
                return true;
            }

            void CleanupDeviceD3D() {
                if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
                if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
                if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
                if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
            }

            void SetWindowClickThrough(HWND hwnd, bool clickThrough) {
                if (clickThrough) {

                    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_LAYERED);
                }
                else {

                    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
                }
            }

            struct ESPConfig {
                bool box = true;
                bool healthBar = true;
                bool name = true;
                bool healthText = true;
                bool skeleton = true;
                bool snapline = false;
                bool distance = true;
            };

            ESPConfig espConfig;

            void DrawESP(const Entity& entity, const ViewMatrix& viewMatrix, int screenWidth, int screenHeight, Memory& mem) {
                Vec2 headScreenPos = WorldToScreen(viewMatrix, entity.head, screenWidth, screenHeight);
                Vec2 feetScreenPos = WorldToScreen(viewMatrix, entity.origin, screenWidth, screenHeight);

                if (headScreenPos.x == -1 || headScreenPos.y == -1 || feetScreenPos.y == -1) return;

                float height = feetScreenPos.y - headScreenPos.y;
                float width = height / 3.0f;
                float boxTop = headScreenPos.y - height * 0.2f;
                float boxHeight = height * 1.4f;

                ImDrawList* drawList = ImGui::GetBackgroundDrawList();
                const ImU32 col = IM_COL32(255, 255, 255, 255);

                // Box ESP
                if (espConfig.box) {
                    drawList->AddRect(ImVec2(headScreenPos.x - width, boxTop),
                        ImVec2(headScreenPos.x + width, boxTop + boxHeight),
                        IM_COL32(255, 0, 0, 255));
                }

                // Health Bar
                if (espConfig.healthBar) {
                    const float healthPercentage = entity.health / 100.0f;
                    const ImVec2 barStart = ImVec2(headScreenPos.x - width - 5, boxTop);
                    const ImVec2 barEnd = ImVec2(barStart.x - 3, barStart.y + boxHeight);
                    const float healthHeight = boxHeight * healthPercentage;

                    // Health bar background
                    drawList->AddRectFilled(barStart, barEnd, IM_COL32(40, 40, 40, 255));

                    // Actual health
                    drawList->AddRectFilled(
                        ImVec2(barStart.x, barEnd.y - healthHeight),
                        barEnd,
                        IM_COL32(0, 255, 0, 255)
                    );
                }

                // Name
                if (espConfig.name && !entity.name.empty()) {
                    const std::string& nameStr = entity.name;
                    const auto textSize = ImGui::CalcTextSize(nameStr.c_str());
                    drawList->AddText(
                        ImVec2(headScreenPos.x - textSize.x / 2, boxTop - textSize.y - 2), // Position above the box
                        IM_COL32(255, 255, 255, 255), // White color
                        nameStr.c_str()
                    );
                }

                // Health Text
                if (espConfig.healthText) {
                    const std::string healthStr = std::to_string(entity.health) + " HP";
                    const auto textSize = ImGui::CalcTextSize(healthStr.c_str());
                    drawList->AddText(
                        ImVec2(headScreenPos.x - textSize.x / 2, boxTop + boxHeight + 2),
                        col,
                        healthStr.c_str()
                    );
                }

                // Distance
                if (espConfig.distance) {
                    const std::string distStr = std::to_string((int)entity.distance) + "m";
                    const auto textSize = ImGui::CalcTextSize(distStr.c_str());
                    drawList->AddText(
                        ImVec2(headScreenPos.x - textSize.x / 2, boxTop + boxHeight + (espConfig.healthText ? 16 : 2)),
                        col,
                        distStr.c_str()
                    );
                }

                // Snapline
                if (espConfig.snapline) {
                    drawList->AddLine(
                        ImVec2(screenWidth / 2, screenHeight),
                        ImVec2(headScreenPos.x, feetScreenPos.y),
                        IM_COL32(255, 255, 255, 255)
                    );
                }

                // Skeleton (Bone ESP)
                if (espConfig.skeleton && !entity.bones.empty()) {
                    const std::vector<std::pair<int, int>> boneConnections = {

                        {6, 5},   // Head to Neck
                        {5, 4},   // Neck to Upper Chest
                        {4, 3},   // Upper Chest to Lower Chest

                        // Left Arm
                        {4, 8},   // Upper Chest to Left Shoulder
                        {8, 9},   // Left Shoulder to Left Elbow
                        {9, 11},  // Left Elbow to Left Hand

                        // Right Arm
                        {4, 13},  // Upper Chest to Right Shoulder
                        {13, 14}, // Right Shoulder to Right Elbow
                        {14, 16}, // Right Elbow to Right Hand

                        {0,5},

                        // Pelvis to legs
                        {0, 22},  // Pelvis to Left Hip
                        {0, 25},  // Pelvis to Right Hip

                        // Left Leg
                        {22, 23}, // Left Hip to Left Knee
                        {23, 24}, // Left Knee to Left Ankle
                        {24, 29}, // Left Ankle to Left Foot

                        // Right Leg
                        {25, 26}, // Right Hip to Right Knee
                        {26, 27}, // Right Knee to Right Ankle
                        {27, 30}, // Right Ankle to Right Foot

                    };

                    for (const auto& connection : boneConnections) {
                        if (entity.bones.count(connection.first) && entity.bones.count(connection.second)) {
                            Vec2 bone1 = WorldToScreen(viewMatrix, entity.bones.at(connection.first), screenWidth, screenHeight);
                            Vec2 bone2 = WorldToScreen(viewMatrix, entity.bones.at(connection.second), screenWidth, screenHeight);

                            if (bone1.x != -1 && bone1.y != -1 && bone2.x != -1 && bone2.y != -1) {
                                drawList->AddLine(
                                    ImVec2(bone1.x, bone1.y),
                                    ImVec2(bone2.x, bone2.y),
                                    IM_COL32(255, 50, 50, 255),
                                    1.5f
                                );
                            }
                        }
                    }
                }
            }

            bool wasInsertPressed = false;
            bool useSmoothing = false;
            bool guiVisible = false;


            int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

                bool triggerbotEnabled = false;
                bool aimbotEnabled = false;
                bool espEnabled = false;
                int triggerbotDelay = 0; // Initialize delay to 0 ms
                bool isSettingAimKey = false;

                Memory mem(L"cs2.exe", L"client.dll");
                if (!mem.IsValid()) {
                    MessageBox(nullptr, L"Failed to initialize memory.", L"Error", MB_ICONERROR);
                    return 1;
                }

                Aimbot aimbot(mem);
                Triggerbot triggerbot(mem);

                WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, _T("Cheat Menu"), nullptr };
                RegisterClassEx(&wc);

                HWND hwnd = CreateWindowEx(
                    WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
                    wc.lpszClassName,
                    _T("Violet ext"),
                    WS_POPUP,
                    0, 0,
                    Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT,
                    nullptr,
                    nullptr,
                    wc.hInstance,
                    nullptr
                );

                SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);


                if (!CreateDeviceD3D(hwnd)) {
                    CleanupDeviceD3D();
                    UnregisterClass(wc.lpszClassName, wc.hInstance);
                    return 1;
                }

                ShowWindow(hwnd, SW_SHOWDEFAULT);
                UpdateWindow(hwnd);

                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO(); (void)io;


                ApplyModernTheme();

                ImGui_ImplWin32_Init(hwnd);
                ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

                bool done = false;
                bool wasAimKeyPressed = false;

                while (!done) {
                    MSG msg;
                    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        if (msg.message == WM_QUIT)
                            done = true;
                    }
                    if (done)
                        break;

                    Entity localPlayer = aimbot.GetLocalPlayer();
                    HandleNoSmoke(mem, localPlayer);
                    HandleFOVChanger(mem, localPlayer); // Add this line


                    if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
                        if (!wasInsertPressed) {
                            guiVisible = !guiVisible;
                            wasInsertPressed = true;


                            SetWindowClickThrough(hwnd, !guiVisible);
                        }
                    }
                    else {
                        wasInsertPressed = false;
                    }


                    bool isAimKeyPressed = GetAsyncKeyState(Config::AIMKEY) & 0x8000;

                    if (aimbotEnabled && isAimKeyPressed) {
                        Entity localPlayer = aimbot.GetLocalPlayer();
                        std::vector<Entity> entities = aimbot.GetEntities(localPlayer);
                        Entity* target = aimbot.GetBestTarget(entities, localPlayer.origin);

                        if (target) {
                            aimbot.AimAtTarget(target, localPlayer, localPlayer.origin, useSmoothing);
                        }
                    }


                    if (!isAimKeyPressed && wasAimKeyPressed) {
                        aimbot.ResetTarget();
                    }


                    wasAimKeyPressed = isAimKeyPressed;

                    if (triggerbotEnabled) {
                        Entity localPlayer = aimbot.GetLocalPlayer();
                        if (localPlayer.pawnAddress != 0) {
                            triggerbot.Update(localPlayer);
                        }
                    }


                    ImGui_ImplDX11_NewFrame();
                    ImGui_ImplWin32_NewFrame();
                    ImGui::NewFrame();


                    if (guiVisible) {
                        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
                        if (ImGui::Begin("Violet ext", &guiVisible, ImGuiWindowFlags_NoCollapse)) {
                            // Header
                            ImGui::PushFont(io.Fonts->Fonts[0]);
                            ImGui::TextColored(ImVec4(0.48f, 0.28f, 0.85f, 1.00f), "VIOLET EXT");
                            ImGui::PopFont();
                            ImGui::Separator();

                            // Sections
                            if (ImGui::CollapsingHeader("Aimbot Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ImGui::Checkbox("Enable Aimbot", &aimbotEnabled);
                                ImGui::Spacing();

                                ImGui::Checkbox("Use Smoothing", &useSmoothing);
                                if (useSmoothing) {
                                    ImGui::Indent();
                                    ImGui::SliderFloat("Smoothing Factor", &Config::SMOOTHING, 1.0f, 20.0f, "%.1f");
                                    ImGui::Unindent();
                                }

                                ImGui::Spacing();
                                if (ImGui::Button("Set Aim Key", ImVec2(120, 25))) {
                                    isSettingAimKey = true;
                                }
                                ImGui::SameLine();
                                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Current: %s", GetKeyName(Config::AIMKEY).c_str());


                                ImGui::Spacing();
                                ImGui::SliderFloat("Aimbot FOV", &Config::AIM_FOV, 1.0f, 180.0f, "%.1f");

                                ImGui::Spacing();
                                ImGui::Checkbox("Show FOV Circle", &Config::SHOW_FOV_CIRCLE);
                            }

                            if (ImGui::CollapsingHeader("FOV Changer Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ImGui::Checkbox("Enable FOV Changer", &Config::VFOVChangerEnabled);
                                ImGui::Spacing();

                                if (Config::VFOVChangerEnabled) {
                                    ImGui::SliderInt("FOV", &Config::VFOV, 70, 160);
                                }
                            }

                            if (ImGui::CollapsingHeader("Triggerbot Settings")) {
                                if (ImGui::Checkbox("Enable Triggerbot", &triggerbotEnabled)) {
                                    triggerbot.SetEnabled(triggerbotEnabled);
                                }
                                ImGui::Spacing();
                                ImGui::SliderInt("Triggerbot Delay (ms)", &triggerbotDelay, 0, 500); // Adjust the range as needed
                                triggerbot.SetDelay(triggerbotDelay);
                            }

                            if (ImGui::CollapsingHeader("Visual Settings", ImGuiTreeNodeFlags_DefaultOpen)) {

                                ImGui::Checkbox("Show Spectators", &Config::SHOW_SPECTATORS);
                                if (Config::SHOW_SPECTATORS) {
                                    ImGui::Indent();
                                    ImGui::Checkbox("Only Local Spectators", &Config::SPECTATORS_ONLY_LOCAL);
                                    ImGui::Unindent();
                                }

                                ImGui::Checkbox("Enable ESP", &espEnabled);
                                if (espEnabled) {
                                    ImGui::Indent();
                                    ImGui::BeginChild("ESP Columns", ImVec2(0, 140), true);
                                    ImGui::Columns(2, nullptr, false);

                                    ImGui::Checkbox("Box ESP", &espConfig.box);
                                    ImGui::Checkbox("Health Bar", &espConfig.healthBar);
                                    ImGui::Checkbox("Name ESP", &espConfig.name);

                                    ImGui::NextColumn();

                                    ImGui::Checkbox("Health Text", &espConfig.healthText);
                                    ImGui::Checkbox("Distance", &espConfig.distance);
                                    ImGui::Checkbox("Skeleton ESP", &espConfig.skeleton);

                                    ImGui::Columns(1);
                                    ImGui::EndChild();
                                    ImGui::Unindent();

                                    ImGui::Checkbox("No Flash", &Config::NO_FLASH);
                                    ImGui::Checkbox("No Smoke", &Config::NO_SMOKE);

                                }
                            }

                            ImGui::End();
                        }
                    }


                    if (Config::SHOW_FOV_CIRCLE) {
                        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
                        // Center of the screen
                        ImVec2 screenCenter(Config::SCREEN_WIDTH / 2.0f, Config::SCREEN_HEIGHT / 2.0f);
                        // Calculate the radius based on FOV angle
                        float fovRadians = (Config::AIM_FOV / 2.0f) * (3.14159265358979323846f / 180.0f); // Convert FOV to radians
                        // Use a scaling factor to adjust the radius to a reasonable size
                        float distanceToPlane = Config::SCREEN_HEIGHT / 2.0f / tan(70.0f * 0.5f * (3.14159265358979323846f / 180.0f)); // Assume a 70-degree game FOV
                        float radius = tan(fovRadians) * distanceToPlane;
                        // Draw the FOV circle
                        drawList->AddCircle(screenCenter, radius, IM_COL32(255, 255, 255, 255), 100, 1.5f);
                    }

                    if (Config::SHOW_SPECTATORS) {
                        Entity localPlayer = aimbot.GetLocalPlayer();
                        DrawSpectatorList(mem, localPlayer, Config::SCREEN_WIDTH);
                    }

                    if (Config::NO_FLASH && localPlayer.pawnAddress != 0) {
                        mem.Write<float>(localPlayer.pawnAddress + Offsets::m_flFlashMaxAlpha, 0.0f);
                    }

                    // Apply No Smoke
                    if (Config::NO_SMOKE && localPlayer.pawnAddress != 0) {
                        uintptr_t entityList = mem.Read<uintptr_t>(mem.GetModuleBase() + Offsets::dwEntityList);

                        // Iterate through entity list (adjust max index as needed)
                        for (int i = 0; i < 1024; i++) {
                            uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * (i & 0x7FFF) >> 9) + 16);
                            if (!listEntry) continue;

                            uintptr_t entity = mem.Read<uintptr_t>(listEntry + 120 * (i & 0x1FF));
                            if (!entity) continue;

                            // Check if entity is smoke grenade projectile
                            int ownerHandle = mem.Read<int>(entity + Offsets::m_hOwnerEntity);
                            if (ownerHandle == -1) continue;  // Filter out world entities

                            // Set smoke alpha to 0
                            mem.Write<float>(entity + Offsets::m_flLastSmokeOverlayAlpha, 0.0f);
                        }
                    }

                    if (espEnabled) {
                        Entity localPlayer = aimbot.GetLocalPlayer();
                        std::vector<Entity> entities = aimbot.GetEntities(localPlayer);
                        ViewMatrix viewMatrix = mem.Read<ViewMatrix>(mem.GetModuleBase() + Offsets::dwViewMatrix);

                        for (const auto& entity : entities) {
                            if (entity.health > 0 && entity.team != localPlayer.team) {
                                DrawESP(entity, viewMatrix, Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, mem);
                            }
                        }
                    }

                    if (isSettingAimKey) {
                        for (int key = 0; key < 256; key++) {
                            if (key == VK_INSERT) continue; // Skip the menu key

                            bool currentState = (GetAsyncKeyState(key) & 0x8000) != 0;
                            if (currentState && !prevKeyStates[key]) {
                                Config::AIMKEY = key;
                                isSettingAimKey = false;
                                break;
                            }
                            prevKeyStates[key] = currentState;
                        }
                    }
                    else {
                        // Reset previous key states when not setting a key
                        memset(prevKeyStates, 0, sizeof(prevKeyStates));
                    }


                    const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
                    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);


                    ImGui::Render();
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


                    g_pSwapChain->Present(1, 0);
                }

                ImGui_ImplDX11_Shutdown();
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();

                CleanupDeviceD3D();
                DestroyWindow(hwnd);
                UnregisterClass(wc.lpszClassName, wc.hInstance);

                return 0;
            }

            LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
                if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
                    return true;

                switch (msg) {
                case WM_SIZE:
                    if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                        CleanupRenderTarget();
                        g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                        CreateRenderTarget();
                    }
                    return 0;
                case WM_SYSCOMMAND:
                    if ((wParam & 0xfff0) == SC_KEYMENU)
                        return 0;
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
                case WM_NCHITTEST: {
                    POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                    ScreenToClient(hWnd, &pt);

                    if (guiVisible && ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                        return HTCLIENT;
                    }
                    return HTTRANSPARENT;
                }
                }
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }