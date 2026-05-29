#pragma once
#include "DirectXCore.h"
#include "Renderer/Resource/BasicResource.h"
#include "DescriptorManager/SrvManager/SrvManager.h"

template<typename T>
class InstanceBuffer
{
public:

	InstanceBuffer(DirectXCore* directXDriver) {

		directXDriver_ = directXDriver;

	}
	~InstanceBuffer() {

		if (tileResource_) {
			tileResource_->ClearResource();

			SrvManager* srvManager = SrvManager::GetInstance();
			srvManager->Free(srvHandleIndex_);
		}

	}

	T* CreateInstanceBuffer(int bufferSize) {

		/// すでにバッファが存在する場合は、リソースを解放してから新しいバッファを作成する
		if (!isNull_) {
			if (tileResource_) {
				tileResource_->ClearResource();
			}
			instanceBufferPtr_ = nullptr;
			isNull_ = true;
			srvHandleIndex_ = -1;
		}

		bufferSize_ = bufferSize;

		/// SRV Managerを準備する
		SrvManager* srvManager = SrvManager::GetInstance();

		/// Resourceを作成する
		tileResource_ = std::make_unique<BasicResource>();
		tileResource_->CreateResourceClass_(directXDriver_->GetDevice(), (sizeof(T) * bufferSize));

		/// Ptrをnullにしておく
		instanceBufferPtr_ = nullptr;

		/// SRVを作成して、CPUとGPUのハンドルを保存する
		srvHandleIndex_ = srvManager->Allocate();
		tileResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&instanceBufferPtr_));

		srvManager->CreateSRVForStructuredBuffer(
			(uint32_t)srvHandleIndex_,
			tileResource_->GetResource().Get(),
			(UINT)bufferSize_,
			(UINT)sizeof(T));

		descriptorHandle_.first = srvManager->GetCPUDescriptorHandle(srvHandleIndex_);
		descriptorHandle_.second = srvManager->GetGPUDescriptorHandle(srvHandleIndex_);

		/// nullじゃないフラグを立て,Ptrを返す
		isNull_ = false;
		return instanceBufferPtr_;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() {
		return descriptorHandle_.first;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() {
		return descriptorHandle_.second;
	}

	int GetCount() const { return bufferSize_; }

	BasicResource* GetResource() const { return tileResource_.get(); }

	bool IsNull() { return isNull_; }

private:

	DirectXCore* directXDriver_{}; // 借りのDirectXDriver

	/// インスタンスバッファのCPU側ポインタ
	T* instanceBufferPtr_ = nullptr; // Instanceだから、ここで管理ではなく、DrawDataCollectorで管理する(元々はInstanceManagerの仕事)

	/// タイル描画用のインスタンスバッファ（前に作ったBasicResourceに既にセーブしてる）
	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> descriptorHandle_;

	/// タイル描画用のWVP行列を収納するResource
	std::unique_ptr<BasicResource> tileResource_;

	/// srvHandle
	int srvHandleIndex_ = -1;

	/// インスタンスバッファのカウンター
	int bufferSize_ = 0;

	/// nullじゃないフラグ
	bool isNull_ = true;
};

