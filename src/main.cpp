#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <string>

// ImGui Headers
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

// Your Custom Headers
#include "memory.h"
#include "cheat.h"

// Link DirectX libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

// Global variables for ImGui/DirectX
ID3D11Device* device = nullptr;
ID3D11DeviceContext* device_context = nullptr;
IDXGISwapChain* swap_chain = nullptr;
ID3D11RenderTargetView* render_target_view = nullptr;

// External handler for Win32 window messages
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window Procedure
LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
        return 0L;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0L;
    }

    return DefWindowProc(window, message, w_param, l_param);
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {
    // 1. Initialize Memory
    Memory mem("ac_client.exe");
    uintptr_t client = 0;

    // Wait for the game to open if it's not already
    while (client == 0) {
        client = mem.GetModuleAddress("ac_client.exe");
        Sleep(500);
    }

    // 2. Define Window Class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_procedure;
    wc.hInstance = instance;
    wc.lpszClassName = L"OverlayClass";
    RegisterClassExW(&wc);

    // 3. Create the Transparent Window
    const HWND window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        L"AssaultCube External",
        WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
    const MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(window, &margins);

    // 4. Initialize DirectX 11
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferDesc.RefreshRate.Numerator = 60U;
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &device, nullptr, &device_context);

    ID3D11Texture2D* back_buffer = nullptr;
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (back_buffer) {
        device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
        back_buffer->Release();
    }

    // 5. Initialize ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);

    ShowWindow(window, cmd_show);

    // Cheat States
    bool bESP = true;
    bool bMenuOpen = true;

    // 6. Main Loop
    bool running = true;
    while (running) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        if (!running) break;

        // Toggle menu with Insert key
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            bMenuOpen = !bMenuOpen;
            // Update window styles for click-through
            long style = GetWindowLong(window, GWL_EXSTYLE);
            if (bMenuOpen) {
                SetWindowLong(window, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
            }
            else {
                SetWindowLong(window, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
            }
        }

        // Start ImGui Frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // --- Hack Logic Start ---
        if (bMenuOpen) {
            ImGui::Begin("Adrian's AC External", &bMenuOpen, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("ESP Boxes", &bESP);
            ImGui::Text("Press INSERT to toggle menu");
            ImGui::End();
        }

        // ESP Implementation
        if (bESP) {
            float vMatrix[16];
            mem.Read(client + offsets::ViewMatrix, &vMatrix, sizeof(vMatrix));

            uintptr_t localPlayer = mem.Read<uintptr_t>(client + offsets::localPlayer);
            uintptr_t entityList = mem.Read<uintptr_t>(client + offsets::EntityList);
            int playerCount = mem.Read<int>(client + offsets::PlayerCount);

            for (int i = 1; i < playerCount; i++) {
                uintptr_t entity = mem.Read<uintptr_t>(entityList + (i * 0x4));
                if (!entity) continue;

                int health = mem.Read<int>(entity + offsets::health);
                if (health <= 0 || health > 100) continue;

                Vector3 feetPos = {
                    mem.Read<float>(entity + offsets::PPosX),
                    mem.Read<float>(entity + offsets::PPosY),
                    mem.Read<float>(entity + offsets::PPosZ)
                };
                Vector3 headPos = { feetPos.x, feetPos.y, feetPos.z + 4.5f };

                Vector2 sFeet, sHead;
                int screenW = GetSystemMetrics(SM_CXSCREEN);
                int screenH = GetSystemMetrics(SM_CYSCREEN);

                if (WorldToScreen(feetPos, sFeet, vMatrix, screenW, screenH) &&
                    WorldToScreen(headPos, sHead, vMatrix, screenW, screenH)) {

                    float h = abs(sFeet.y - sHead.y);
                    float w = h / 2.0f;

                    ImGui::GetBackgroundDrawList()->AddRect(
                        { sHead.x - w / 2, sHead.y },
                        { sHead.x + w / 2, sFeet.y },
                        ImColor(255, 0, 0), // Red box
                        0.0f, 0, 1.5f
                    );
                }
            }
        }
        // --- Hack Logic End ---

        // Render ImGui
        ImGui::Render();
        const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_context->ClearRenderTargetView(render_target_view, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swap_chain->Present(1U, 0U);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (swap_chain) swap_chain->Release();
    if (device_context) device_context->Release();
    if (device) device->Release();
    if (render_target_view) render_target_view->Release();

    return 0;
}