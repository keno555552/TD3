#include "SrvManager.h"
#include "DirectXCore.h"
#include <cassert>


/// ========== グローバル定数宣言 ========== ///
const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(DirectXCore* core) {
	
	/// コアを保存
	directXCore_ = core;

	// デスクリプタヒープの生成
	descriptorHeap = directXCore_->CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		kMaxSRVCount,
		true
	);

	// デスクリプタ1個分のサイズを取得して記録
	descriptorSize = directXCore_->GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);
}

SrvManager::~SrvManager() {
}

void SrvManager::Finalize() {
	descriptorHeap.Reset();
	freeIndices.clear();
	nextNewIndex = 0;
}


uint32_t SrvManager::Allocate() {

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
	assert(nextNewIndex < kMaxSRVCount);

	/// 上で記録した番号をreturn
	return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

uint32_t SrvManager::GetDescriptorSizeSRV() {
	return descriptorSize;
}

void SrvManager::CreateSRVForTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーでのコンポーネントマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = MipLevels; // 最初のMipLevelを使用

	directXCore_->GetDevice()->CreateShaderResourceView(
		pResource,
		&srvDesc,
		GetCPUDescriptorHandle(srvIndex)
	);
}

void SrvManager::CreateSRVForStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride) {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = structureByteStride;

	directXCore_->GetDevice()->CreateShaderResourceView(
		pResource,
		&srvDesc,
		GetCPUDescriptorHandle(srvIndex)
	);
}

void SrvManager::Free(uint32_t srvIndex) {
	// 廃棄されたSRVインデックスをfreeIndicesに追加
	freeIndices.push_back(srvIndex);
}

bool SrvManager::CheckSRVHeapFull() {
	if (nextNewIndex >= kMaxSRVCount) return true;
	return false;
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex) {
	directXCore_->GetCommandList()->SetGraphicsRootDescriptorTable(
		RootParameterIndex,
		GetGPUDescriptorHandle(srvIndex));
}

