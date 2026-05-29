#include "RtvManager.h"
#include "DirectXCore.h"
#include <cassert>


/// ========== グローバル定数宣言 ========== ///
const uint32_t RtvManager::kMaxRTVCount = 128;
std::unique_ptr <RtvManager> RtvManager::instance_ = nullptr;

RtvManager* RtvManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = std::make_unique<RtvManager>(ConstructorKey{});
		//instance_.reset(new ResourceManager);
	}
	return instance_.get();
}


void RtvManager::Initialize(DirectXCore* core) {

	/// コアを保存
	directXCore_ = core;

	/// RTV Heap
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = kMaxRTVCount;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // RTV 不允許 shader visible

	/// RTV用のデスクリプタヒープを生成して保存
	HRESULT hr = directXCore_->GetDevice()->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(&descriptorHeap)
	);
	assert(SUCCEEDED(hr));

	/// デスクリプタ1個分のサイズを取得して記録
	descriptorSize_ = directXCore_->GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
}

void RtvManager::Finalize() {
	descriptorHeap.Reset();
	freeIndices.clear();
	nextNewIndex = 0;
}

void RtvManager::Destroy() {
	instance_.reset();
}


uint32_t RtvManager::Allocate() {

	/// freeIndicesに廃棄された番号があればそれを返す
	if (!freeIndices.empty()) {
		uint32_t index = freeIndices.back();
		freeIndices.pop_back();
		return index;
	}

	/// return する番号を一旦記録しておく
	int index = nextNewIndex;

	/// 次回のために番号を1進める
	nextNewIndex++;

	/// 上限に達していないかチェックしてassert
	assert(nextNewIndex < kMaxRTVCount);

	/// 上で記録した番号をreturn
	return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE RtvManager::GetCPUDescriptorHandle(uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize_ * index);
	return handleCPU;
}

uint32_t RtvManager::GetDescriptorSizeRTV() {
	return descriptorSize_;
}


void RtvManager::Free(uint32_t srvIndex) {
	// 廃棄されたSRVインデックスをfreeIndicesに追加
	freeIndices.push_back(srvIndex);
}

bool RtvManager::CheckRTVHeapFull() {
	if (nextNewIndex >= kMaxRTVCount) return true;
	return false;
}
