#include "DsvManager.h"
#include "DirectXCore.h"
#include <cassert>


/// ========== グローバル定数宣言 ========== ///
const uint32_t DsvManager::kMaxDSVCount = 128;
std::unique_ptr <DsvManager> DsvManager::instance_ = nullptr;

DsvManager* DsvManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = std::make_unique<DsvManager>(ConstructorKey{});
		//instance_.reset(new ResourceManager);
	}
	return instance_.get();
}


void DsvManager::Initialize(DirectXCore* core) {

	/// コアを保存
	directXCore_ = core;

	/// DSV Heap
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = kMaxDSVCount;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // DSV 不允許 shader visible

	/// DSV用のデスクリプタヒープを生成して保存
	HRESULT hr = directXCore_->GetDevice()->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(&descriptorHeap)
	);
	assert(SUCCEEDED(hr));

	/// デスクリプタ1個分のサイズを取得して記録
	descriptorSize_ = directXCore_->GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	);
}

void DsvManager::Finalize() {
	descriptorHeap.Reset();
	freeIndices.clear();
	nextNewIndex = 0;
}

void DsvManager::Destroy() {
	instance_.reset();
}


uint32_t DsvManager::Allocate() {

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
	assert(nextNewIndex < kMaxDSVCount);

	/// 上で記録した番号をreturn
	return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE DsvManager::GetCPUDescriptorHandle(uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize_ * index);
	return handleCPU;
}

uint32_t DsvManager::GetDescriptorSizeDSV() {
	return descriptorSize_;
}


void DsvManager::Free(uint32_t srvIndex) {
	// 廃棄されたSRVインデックスをfreeIndicesに追加
	freeIndices.push_back(srvIndex);
}

bool DsvManager::CheckDSVHeapFull() {
	if (nextNewIndex >= kMaxDSVCount) return true;
	return false;
}
