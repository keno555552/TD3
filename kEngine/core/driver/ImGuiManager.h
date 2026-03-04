#pragma once

#include <d3d12.h>

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_stdlib.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class DirectXController;
class SrvManager;

class ImGuiManager {
public:
    static bool HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static void Initialize(DirectXController* dxComm, SrvManager* srvManager);
    static void BeginFrame();
    static void EndFrame(ID3D12GraphicsCommandList* commandList);
    static void Shutdown();
};

