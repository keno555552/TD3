#include "ImGuiManager.h"
#include "DirectXController.h"
#include "SrvManager.h"

bool ImGuiManager::HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
}

void ImGuiManager::Initialize(DirectXController* dxComm, SrvManager* srvManager) {
    uint32_t srvIndex = srvManager->Allocate();
    dxComm->GetCommandQueue();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(dxComm->GetHWND());
    ImGui_ImplDX12_Init(
        dxComm->GetDevice(),
        2,
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        srvManager->GetDescriptorHeap(),
        srvManager->GetCPUDescriptorHandle(srvIndex),
        srvManager->GetGPUDescriptorHandle(srvIndex));
}

/// IMGUI_CHECKVERSION();
/// ImGui::CreateContext();
/// ImGui::StyleColorsDark();
/// ImGui_ImplWin32_Init(winAPI_->GetHWND());
/// ImGui_ImplDX12_Init(
///     device,
/// 	swapChainDesc.BufferCount,
/// 	rtvDesc.Format,
/// 	srvDescriptorHeap,
/// 	srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
/// 	srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

void ImGuiManager::BeginFrame() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList) {
    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiManager::Shutdown() {
    /// ImGuiの終了処理。詳細はさして重要ではないので解説は省略する。
// こういうもんである。初期化と逆順に行う
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
