#include "DirectXController.h"
#include <pplwin.h>
#include <cassert>

DirectXController::DirectXController() {
	//static FixFPS fixFPS;
	//fixFPS.Initialize();
	//
	//timeBeginPeriod(1);
}

DirectXController::~DirectXController() {

	//timeEndPeriod(1);
	//Finalize();
}

void DirectXController::InitializeDrive(const char* kClientTitle, int kClientWidth, int kClientHeight) {
	DirectXCore::InitializeDrive(kClientTitle, kClientWidth, kClientHeight);
	static FixFPS fixFPS;
	fixFPS.Initialize();
	timeBeginPeriod(1);
}

void DirectXController::Finalize() {
	timeEndPeriod(1);
	DirectXCore::Finalize();
}

void DirectXController::StartFrame() {

#ifdef USE_IMGUI
	ImGuiManager::BeginFrame();
#endif

	// これから書き込むバックバッファのインデックスを取得
	UINT backBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	// TransitionBarrierの設定
	//D3D12_RESOURCE_BARRIER barrier{};
	barrier = {};
	// 今回のバリアはTransition
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	// Noneにしておく
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	// バリアを張る対象のリソース。現在のバックバッファに対して行う
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
	// 遷移前（現在）のResourceState
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	// 遷移後のResourceState
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);

	// 描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
	// 指定した色で画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // 青っぽい色。RGBAの順
	//float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 黑色。RGBAの順
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
}

void DirectXController::EndFrame() {

#ifdef _DEBUG
#ifdef USE_IMGUI
	ImGuiManager::EndFrame(commandList.Get());
#endif
#endif

	UINT backBufferIndex = SwapChain->GetCurrentBackBufferIndex();

	// 画面に描く処理はすべて終わり、画面に映すので、状態を遷移
	// 今回はRenderTargetからPresentにする
	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();


	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;


	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// TransitionBarrierを張る
	commandList->ResourceBarrier(1, &barrier);

	// コマンドリストの内容を確定させる。すべてのコマンドを積んでからCloseすること
	HRESULT hr = commandList->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList.Get()};
	commandQueue->ExecuteCommandLists(1, commandLists);

	// GPUとOSに画面の交換を行うよう通知する
	SwapChain->Present(1, 0);

	//static FixFPS fixFPS;
	//fixFPS.Update();


	// Fenceの値を更新
	fenceValue++;
	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue->Signal(fence.Get(), fenceValue);

	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence->GetCompletedValue() < fenceValue) {
		// 指定したSignalにたどりついていないので、たどり着くまで待つようにイベントを設定する
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		// イベント待つ
		WaitForSingleObject(fenceEvent, INFINITE);
	}


	// 次のフレーム用のコマンドリストを準備
	hr = commandAllocator->Reset(); // 重置命令分配器，為下一幀準備
	assert(SUCCEEDED(hr)); // チェック
	hr = commandList->Reset(commandAllocator.Get(), nullptr); // コマンドリストをリセット
	assert(SUCCEEDED(hr)); // チェック

}