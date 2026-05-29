#pragma once
#include <wrl/client.h>
#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <memory>

#include "externals/DirectXTex/DirectXTex.h"

class DirectXCore;
class DsvManager
{
public:

	/// シングルトン取得
	static DsvManager* GetInstance();

	class ConstructorKey {
	private:
		/// からのみ生成・破棄可能
		friend class DsvManager;
		friend class kEngine;
		ConstructorKey() {}
	};

	explicit DsvManager(ConstructorKey) {};

	void Initialize(DirectXCore* core);
	void Finalize();

	static void Destroy();

	uint32_t Allocate();

	/// CPU,GPUハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);


	/// =========== DSV生成・解放 ===========///
	/// DSV解放
	void Free(uint32_t srvIndex);

	/// DSV heap 檢查
	bool CheckDSVHeapFull();

	/// 取得 descriptor size
	uint32_t GetDescriptorSizeDSV();

	/// 取得 descriptor heap
	ID3D12DescriptorHeap* GetDescriptorHeap() { return descriptorHeap.Get(); }

private:

	friend struct std::default_delete<DsvManager>;
	~DsvManager() = default;

private:

	/// singletonドライブ
	static std::unique_ptr<DsvManager> instance_;

	/// 借りのDevice
	DirectXCore* directXCore_ = nullptr; //借り

	/// 最大DSV数（最大テクスチャ枚数）
	static const uint32_t kMaxDSVCount;

	/// DSV用のデスクリプタサイズ
	uint32_t descriptorSize_{};

	/// DSV用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap{};

	/// 廃棄DSV管理配列
	std::vector<uint32_t> freeIndices{};

	// 次に使用するDSVインデックス
	uint32_t nextNewIndex = 0;

};

// TODO: Freeはあるが、それを再利用する機能がない。時間があればやる。
// TODO: DSVヒープのサイズ検査がない。DSVヒープがいっぱいになったらエラーを出すようにする.



