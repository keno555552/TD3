#pragma once
#include "DirectXCore.h"
#include "SrvManager.h"
#include "BasicResource.h"
#include "externals/DirectXTex/DirectXTex.h"
//#include "WinAPI.h"
#include <wrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "config.h"

class TextureManager
{
public:
	/// シングルトン取得
	static TextureManager* GetInstance();

	/// 初期化
	void Initialize(DirectXCore* core, SrvManager* srvManager);
	/// シングルトン解放
	void Finalize();
	/// 
	static void Destroy();

	/// <summary>
	/// 一般のテクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath">テクスチャファイルのパス</param>
	/// <returns>テキスチャハンドル</returns>
	int LoadCommonTexture(const std::string& filePath);

	/// <summary>
	/// モデルのテクスチャファイルの読み込み
	/// </summary>
	/// <param name="filePath">テクスチャファイルのパス</param>
	/// <returns>テキスチャハンドル</returns>
	int LoadModelTexture(const std::string& filePath);


	/// <summary>
	/// 
	/// </summary>
	/// <param name="textureHandle"></param>
	/// <returns></returns>
	DirectX::TexMetadata GetTextureMetadata(int textureHandle);

	int GetCommonTextureHandle(int textureHandle);

	int GetModelTextureHandle(int textureHandle);

	int CheckSameCommonTextureLoaded(const std::string& filePath);

	int CheckSameModelTextureLoaded(const std::string& filePath);

	static int GetDefaultTextureHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE GetTextureCPUDescriptorHandle(int textureHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureGPUDescriptorHandle(int textureHandle);


	int GetTextureCounter() { return descriptorIndex_; }
	void TextureCounterPlus(int index) { descriptorIndex_ += index; }
	void TextuerCounterAdjust(int index) { descriptorIndex_ = index;};

	void EndUploadingTexture();

private:

	/// すべきのテクスチャハンドル
	static int nextTextureHandle_;

	/// テクスチャ1枚分のデータ
	struct TextureData {
		DirectX::TexMetadata metadata{};
		std::string filePath{};
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex{};
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};
	};

	std::unordered_map <int, TextureData> textureDatas;

	BasicResource* intermediateResource_ = new BasicResource;/*EndDrawでTextrueを作ったら解放する*/


private:
	/// 借りのDevice
	static DirectXCore* core_;
	static ID3D12Device* device_;
	static SrvManager* srvManager_;

	/// シングルトンインスタンス
	static TextureManager* instance_;

	TextureManager() = default;
	~TextureManager() = default;

	static uint32_t descriptorIndex_;


	/// ファイルパスからテクスチャハンドルへのマップ
	std::unordered_map<std::string, int> filePathToHandle_;
	std::unordered_map<int, uint32_t> commonTextureSRVMap_;
	std::unordered_map<int, uint32_t> modelTextureSRVMap_;

	/// デフォルトテクスチャハンドル
	static const int defaultTextureHandle_ = 0;

private:
	ID3D12Resource* CreateTextureResource(const DirectX::TexMetadata& metadata);

	uint32_t MakeCommonTextureShaderResourceView(TextureData* textureData);

	uint32_t MakeModelTextureShaderResourceView(TextureData* textureData);

	ID3D12Resource* UploadTextureData(const DirectX::ScratchImage& mipImages, TextureData* textureData);

};

