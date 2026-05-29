#pragma once
#include <wrl/client.h>
#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <memory>

#include "externals/DirectXTex/DirectXTex.h"

class DirectXCore;
class RtvManager
{
public:

	/// シングルトン取得
	static RtvManager* GetInstance();

	class ConstructorKey {
	private:
		/// からのみ生成・破棄可能
		friend class RtvManager;
		friend class kEngine;
		ConstructorKey() {}
	};

	explicit RtvManager(ConstructorKey) {};

	void Initialize(DirectXCore* core);
	void Finalize();

	static void Destroy();

	uint32_t Allocate();

	/// CPU,GPUハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);


	/// =========== RTV生成・解放 ===========///
	/// RTV解放
	void Free(uint32_t srvIndex);

	/// RTV heap 檢查
	bool CheckRTVHeapFull();

	/// 取得 descriptor size
	uint32_t GetDescriptorSizeRTV();

	/// 取得 descriptor heap
	ID3D12DescriptorHeap* GetDescriptorHeap() { return descriptorHeap.Get(); }

private:

	friend struct std::default_delete<RtvManager>;
	~RtvManager() = default;

private:

	/// singletonドライブ
	static std::unique_ptr<RtvManager> instance_;

	/// 借りのDevice
	DirectXCore* directXCore_ = nullptr; //借り

	/// 最大RTV数（最大テクスチャ枚数）
	static const uint32_t kMaxRTVCount;

	/// RTV用のデスクリプタサイズ
	uint32_t descriptorSize_{};

	/// RTV用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap{};

	/// 廃棄RTV管理配列
	std::vector<uint32_t> freeIndices{};

	// 次に使用するRTVインデックス
	uint32_t nextNewIndex = 0;

};

// TODO: Freeはあるが、それを再利用する機能がない。時間があればやる。
// TODO: RTVヒープのサイズ検査がない。RTVヒープがいっぱいになったらエラーを出すようにする.



