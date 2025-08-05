#define IMGUI_BACKEND_HAS_WINDOWS_H
#include <d3d11.h>
#include <dwmapi.h>
#include <windows.h>

#include "dcimgui.h"
#include "dcimgui_impl_dx11.h"
#include "dcimgui_impl_win32.h"

#include <tchar.h>
#include "overlay.h"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static bool                     g_SwapChainOccluded = false;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
void OV_Render(void) {
    // Create application window
    typedef HWND(WINAPI* CreateWindowInBand)(_In_ DWORD dwExStyle, _In_opt_ ATOM atom, _In_opt_ LPCWSTR lpWindowName, _In_ DWORD dwStyle,
        _In_ int X, _In_ int Y, _In_ int nWidth, _In_ int nHeight, _In_opt_ HWND hWndParent, _In_opt_ HMENU hMenu, _In_opt_ HINSTANCE hInstance,
        _In_opt_ LPVOID lpParam, DWORD band
    );

    CreateWindowInBand pCreateWindowInBand = (CreateWindowInBand)(GetProcAddress(LoadLibraryA("user32.dll"), "CreateWindowInBand"));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL,
        NULL, NULL, NULL, L"OverlayClass", NULL
    };
    ATOM res = RegisterClassExW(&wc);

    HWND hwnd = pCreateWindowInBand(WS_EX_TOPMOST, res, L"Overlay Example", WS_POPUP,
        0, 0, 1024, 786, NULL, NULL, wc.hInstance, NULL, ZBID_UIACCESS
    ); // GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margin);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup Dear ImGui context
    ImGui_CreateContext(NULL);

    ImGuiIO* io = ImGui_GetIO(); (void)io;

    // Disable file creating
    io->IniFilename = NULL;
    io->LogFilename = NULL;

    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui_StyleColorsClassic(NULL);

    // Setup Platform/Renderer backends
    cImGui_ImplWin32_Init(hwnd);
    cImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Our state
    ImVec4 clearColor = {0.f, 0.f, 0.f, 0.f};

    // Main loop
    bool running = true;
    while (running) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }

        if (!running)
            break;

        if (g_SwapChainOccluded && g_pSwapChain->lpVtbl->Present(g_pSwapChain, 0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
            CleanupRenderTarget();
            g_pSwapChain->lpVtbl->ResizeBuffers(g_pSwapChain, 0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        SetWindowLongA(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT);

        // Start the Dear ImGui frame
        cImGui_ImplDX11_NewFrame();
        cImGui_ImplWin32_NewFrame();
        ImGui_NewFrame();


        // --- Render here --- //
        ImU32 color1 = IM_COL32(255, 0, 0, 255);
        ImU32 color2 = IM_COL32(0, 255, 0, 255);
        ImU32 color3 = IM_COL32(0, 0, 255, 255);
        ImU32 color4 = IM_COL32(0, 0, 0, 255);
        ImDrawList_AddRectFilledMultiColor(ImGui_GetBackgroundDrawList(), (ImVec2){150, 150}, (ImVec2){250, 250}, color1, color2, color3, color4);

        ImDrawList_AddText(ImGui_GetBackgroundDrawList(), (ImVec2){150, 130}, IM_COL32(255, 255, 255, 255), "Some Text");


        // Rendering
        ImGui_Render();

        const float clearColorWithAlpha[4] = { clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w };
        g_pd3dDeviceContext->lpVtbl->OMSetRenderTargets(g_pd3dDeviceContext, 1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->lpVtbl->ClearRenderTargetView(g_pd3dDeviceContext, g_mainRenderTargetView, clearColorWithAlpha);
        cImGui_ImplDX11_RenderDrawData(ImGui_GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->lpVtbl->Present(g_pSwapChain, 1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    cImGui_ImplDX11_Shutdown();
    cImGui_ImplWin32_Shutdown();
    ImGui_DestroyContext(NULL);

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd) {
    // Setup swap chain
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->lpVtbl->Release(g_pSwapChain); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->lpVtbl->Release(g_pd3dDeviceContext); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->lpVtbl->Release(g_pd3dDevice); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->lpVtbl->GetBuffer(g_pSwapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    g_pd3dDevice->lpVtbl->CreateRenderTargetView(g_pd3dDevice, (ID3D11Resource*)pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->lpVtbl->Release(pBackBuffer);
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->lpVtbl->Release(g_mainRenderTargetView); g_mainRenderTargetView = nullptr; }
}

extern CIMGUI_IMPL_API LRESULT cImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (cImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED)
                return 0;

            g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
            g_ResizeHeight = (UINT)HIWORD(lParam);

            return 0;
        }
        case WM_SYSCOMMAND: {
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}