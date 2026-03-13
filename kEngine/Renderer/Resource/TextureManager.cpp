#include "TextureManager.h"
#include <externals/DirectXTex/d3dx12.h>
#include <cassert>

DirectXCore* TextureManager::core_ = nullptr;
ID3D12Device* TextureManager::device_ = nullptr;
SrvManager* TextureManager::srvManager_ = nullptr;
std::unique_ptr <TextureManager> TextureManager::instance_ = nullptr;
uint32_t TextureManager::descriptorIndex_ = 0;
int TextureManager::nextTextureHandle_ = 0;  // 添加缺失的靜態成員定義

int TextureManager::GetDefaultTextureHandle() { return defaultTextureHandle_; }

void TextureManager::EndUploadingTexture() {
	if (intermediateResource_->GetResourceCounter() != 0)intermediateResource_->ClearResource();
}

//TextureManager* TextureManager::GetInstance() {
//	if (instance_ == nullptr) {
//		instance_ = std::make_unique<TextureManager>();
//	}
//	return instance_.get();
//}
TextureManager* TextureManager::GetInstance() {
	if (!instance_) {
		instance_.reset(new TextureManager());
	}
	return instance_.get();
}


void TextureManager::Initialize(DirectXCore* core, SrvManager* srvManager)
{
	core_ = core;
	device_ = core->GetDevice();
	srvManager_ = srvManager;
	//textureDatas.reserve(config::GetMaxSRVNum());
	descriptorIndex_ = 0;
	nextTextureHandle_ = 0;
	intermediateResource_ = std::make_unique<BasicResource>();
}

void TextureManager::Finalize() {

	for (auto& [handle, tex] : textureDatas) {
		if (srvManager_ && tex.srvIndex != 0) {
			srvManager_->Free(tex.srvIndex);
		}
		tex.resource.Reset();
	}
	textureDatas.clear();

	filePathToHandle_.clear();
	commonTextureSRVMap_.clear();
	modelTextureSRVMap_.clear();
	descriptorIndex_ = 0;

	if (intermediateResource_) {
		intermediateResource_->ClearResource();
		intermediateResource_.reset();
	}
}

void TextureManager::Destroy() {
	instance_.reset();
}

DirectX::TexMetadata TextureManager::GetTextureMetadata(int textureHandle) {
	assert(textureDatas.contains(textureHandle));
	return textureDatas[textureHandle].metadata;
}

int TextureManager::GetCommonTextureHandle(int textureHandle) {
	assert(commonTextureSRVMap_.contains(textureHandle));
	return commonTextureSRVMap_[textureHandle];
}

int TextureManager::GetModelTextureHandle(int textureHandle) {
	assert(modelTextureSRVMap_.contains(textureHandle));
	return modelTextureSRVMap_[textureHandle];
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetTextureCPUDescriptorHandle(int textureHandle) {
	assert(textureDatas.contains(textureHandle));
	return textureDatas[textureHandle].srvHandleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetTextureGPUDescriptorHandle(int textureHandle) {
	assert(textureDatas.contains(textureHandle));
	return textureDatas[textureHandle].srvHandleGPU;
}

#pragma region CommonTexture

int TextureManager::LoadCommonTexture(const std::string& filePath) {
	/// すでに同じテクスチャが読み込まれているか確認
	int textureHandle = CheckSameCommonTextureLoaded(filePath);
	if (textureHandle != -1)return textureHandle;

	/// WICファイルからテクスチャデータを読み込む
	DirectX::ScratchImage image{};
	HRESULT hr = DirectX::LoadFromWICFile(std::wstring(filePath.begin(), filePath.end()).c_str(),
		DirectX::WIC_FLAGS_NONE, nullptr, image);

	/// ミップマップ生成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
		DirectX::TEX_FILTER_DEFAULT, 0, mipImages);

	/// 追加したテキスチャテータの参照を取得
	auto& textureData = textureDatas[nextTextureHandle_];									/// 新しいテクスチャデータを追加
	textureData.metadata = mipImages.GetMetadata();											/// 1.metadata
	textureData.filePath = filePath;														/// 2.path
	textureData.resource.Attach(CreateTextureResource(textureData.metadata));				/// 3.resource

	/// シェーダーリソースビュー作成とテクスチャデータのアップロード
	textureData.srvIndex = MakeCommonTextureShaderResourceView(&textureData);				/// 4.srvIndex

	/// テクスチャハンドルを更新する
	textureHandle = nextTextureHandle_;

	/// シェーダーリソースビューのハンドル取得
	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);	/// 5.srvHandleCPU
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);	/// 6.srvHandleGPU

	/// テクスチャデータのアップロード
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
	intermediateResource.Attach(UploadTextureData(mipImages, &textureData));
	intermediateResource_->SaveResource_(intermediateResource);;

	/// 次のテクスチャハンドルをインクリメント
	nextTextureHandle_++;

	return textureHandle;
}

int TextureManager::CheckSameCommonTextureLoaded(const std::string& filePath) {

	auto checker = filePathToHandle_.find(filePath);
	if(checker == filePathToHandle_.end())	return -1;
									  else	return checker->second;

}

uint32_t TextureManager::MakeCommonTextureShaderResourceView(TextureData* textureData) {

	/// SRV作成
	uint32_t srvIndex = srvManager_->Allocate();

	srvManager_->CreateSRVForTexture2D(srvIndex, textureData->resource.Get(), textureData->metadata.format, UINT(textureData->metadata.mipLevels));

	/// テクスチャハンドルを設定
	filePathToHandle_[textureData->filePath] = nextTextureHandle_;
	commonTextureSRVMap_[nextTextureHandle_] = srvIndex;

	return srvIndex;
}

#pragma endregion



	#pragma region ModelTexture

	int TextureManager::LoadModelTexture(const std::string& filePath) {
		/// すでに同じテクスチャが読み込まれているか確認
		int textureHandle = CheckSameModelTextureLoaded(filePath);
		if (textureHandle != -1)return textureHandle;

		/// WICファイルからテクスチャデータを読み込む
		DirectX::ScratchImage image{};
		HRESULT hr = DirectX::LoadFromWICFile(std::wstring(filePath.begin(), filePath.end()).c_str(),
			DirectX::WIC_FLAGS_NONE, nullptr, image);

		/// ミップマップ生成
		DirectX::ScratchImage mipImages{};
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
			DirectX::TEX_FILTER_DEFAULT, 0, mipImages);

		/// 追加したテキスチャテータの参照を取得
		auto& textureData = textureDatas[nextTextureHandle_];									/// 新しいテクスチャデータを追加	
		textureData.metadata = mipImages.GetMetadata();											/// 1.metadata
		textureData.filePath = filePath;														/// 2.path
		textureData.resource.Attach(CreateTextureResource(textureData.metadata));				/// 3.resource

		/// シェーダーリソースビュー作成とテクスチャデータのアップロード
		textureData.srvIndex = MakeModelTextureShaderResourceView(&textureData);

		/// テクスチャハンドルを更新する
		textureHandle = nextTextureHandle_;

		/// シェーダーリソースビューのハンドル取得
		textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);	/// 5.srvHandleCPU
		textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);	/// 6.srvHandleGPU


		///// テクスチャデータのアップロード
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
		intermediateResource.Attach(UploadTextureData(mipImages, &textureData));
		intermediateResource_->SaveResource_(intermediateResource);

		/// 次のテクスチャハンドルをインクリメント
		nextTextureHandle_++;

		return textureHandle;
	}

	int TextureManager::CheckSameModelTextureLoaded(const std::string& filePath) {

		auto checker = filePathToHandle_.find(filePath);
		if (checker == filePathToHandle_.end())	return -1;
		else	return checker->second;

	}


	uint32_t TextureManager::MakeModelTextureShaderResourceView(TextureData* textureData) {

		/// SRV作成
		uint32_t srvIndex = srvManager_->Allocate();

		srvManager_->CreateSRVForTexture2D(srvIndex, textureData->resource.Get(), textureData->metadata.format, UINT(textureData->metadata.mipLevels));

		/// テクスチャハンドルを設定
		filePathToHandle_[textureData->filePath] = nextTextureHandle_;
		modelTextureSRVMap_[nextTextureHandle_] = srvIndex;

		return srvIndex;
	}

	#pragma endregion

	ID3D12Resource* TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata) {

		///  textureの元、時関があればこれをセーブして以降使う<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
		/// 1. metadataを基にResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = UINT(metadata.width); // Textureの幅
		resourceDesc.Height = UINT(metadata.height); // Textureの高さ
		resourceDesc.MipLevels = UINT16(metadata.mipLevels); // MipLevelの数
		resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行きor配列Textureの配列数
		resourceDesc.Format = metadata.format; // TextureのFormat
		resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定。
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。普段使ってるのは2次元


		/// 2.利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る
		//heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK; // WriteBackポリシーでCPUアクセス可能
		//heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0; //プロセッサの近くに配置

		/// 3.Resourceを生成する
		ID3D12Resource* resource = nullptr;
		HRESULT hr = device_->CreateCommittedResource(
			&heapProperties,					// Heapの設定
			D3D12_HEAP_FLAG_NONE,				// Heapの特殊な設定。特になし。
			&resourceDesc,						// Resourceの設定
			D3D12_RESOURCE_STATE_COPY_DEST,		// 初回のResourceState。Textureは基本読むだけ
			nullptr,							// Clear最適値。使わないのでnullptr
			IID_PPV_ARGS(&resource));			// 作成するResourceポインタへのポインタ
		assert(SUCCEEDED(hr));

		//char buffer[128];
		//sprintf_s(buffer, "Create resource at %p\n", resource);
		//OutputDebugStringA(buffer);

		return resource;
	}

	[[nodiscard]]//<---c++属性,戻り値が無視されることを防ぐ
	ID3D12Resource* TextureManager::UploadTextureData(const DirectX::ScratchImage& mipImages, TextureData* textureData) {


		DirectX::TexMetadata metadata = textureData->metadata;
		// PrepareUploadを使って、DirectX12用のSubresource配列を作成する
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		DirectX::PrepareUpload(device_, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
		// Subresource配列を使って、コピー元のIntermediateResourceに必要のサイズを計算する
		uint64_t intermediateSize = GetRequiredIntermediateSize(textureData->resource.Get(), 0, UINT(subresources.size()));
		// 計算したサイズでIntermediateResourceを作成する
		ID3D12Resource* intermediateResource = CreateResource(device_, intermediateSize);
		// UpdataSubresourcesを使って、IntermediateResourceにSubresource配列を書き込み,textureに転送し頭む積む
		UpdateSubresources(core_->GetCommandList(), textureData->resource.Get(), intermediateResource, 0, 0, UINT(subresources.size()), subresources.data());
		// Tetureへの転送後は利用できるよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = textureData->resource.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		core_->GetCommandList()->ResourceBarrier(1, &barrier);
		return intermediateResource;
	}
