#pragma once
#include <wrl/client.h>
#include <cstdint>
#include <d3d12.h>
#include <vector>

class DirectXCore;
class SrvManager
{
public:
	~SrvManager();  // 添加解構函式聲明
	
	void Initialize(DirectXCore* core);

	void Finalize();

	uint32_t Allocate();

	/// CPU,GPUハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);


	/// =========== SRV生成・解放 ===========///
	/// /// SRV生成 (バッファ用)
	/// SRV生成 (テクスチャ用)
	void CreateSRVForTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

	/// SRV生成 (Structured Buffer用)
	void CreateSRVForStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	/// SRV解放
	void Free(uint32_t srvIndex);

	/// SRV heap 檢查
	bool CheckSRVHeapFull();
	
	/// 取得 descriptor size
	uint32_t GetDescriptorSizeSRV();
	
	/// 取得 descriptor heap
	ID3D12DescriptorHeap* GetDescriptorHeap() { return descriptorHeap.Get(); }

	/// =========== 描画用コマンドリスト設定 ===========///

	/// RootDescriptorTable設定
	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

private:
	DirectXCore* directXCore_ = nullptr;

	/// 最大SRV数（最大テクスチャ枚数）
	static const uint32_t kMaxSRVCount;

	/// SRV用のデスクリプタサイズ
	uint32_t descriptorSize{};

	/// SRV用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap{};

	/// 廃棄SRV管理配列
	std::vector<uint32_t> freeIndices{};

	// 次に使用するSRVインデックス
	uint32_t nextNewIndex = 0;

};

