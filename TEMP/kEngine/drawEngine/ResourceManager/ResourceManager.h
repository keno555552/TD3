#pragma once
#include "BasicResource.h"
#include "WVPResource.h"
#include "VertexResource.h"
#include "VertexData.h"
#include "Material.h"
#include "MaterialConfig.h"
#include "VertexIndex.h"
#include "InstanceManager.h"
#include "TextureManager.h"
#include "config.h"
#include "DrawData/ObjectData.h"
#include "DrawData/SpriteData.h"
#include "SrvManager.h"
using MaterialID = int;

class ResourceManager
{
public:
	struct TextureInfo {
		ID3D12Resource* texture;
		int width;
		int height;
		Vector4 uvOffset;
	};

#pragma region Instance管理


#pragma endregion

public:

	/// シングルトン取得
	static ResourceManager* GetInstance();

	void Initialize(DirectXCore* device);
	void Finalize();

	static void Destroy();


	/// ランクこと作成するResource
	void CreateTurnResource();

	/// ランクこと解放するResource
	void ClearTurnResource();

public:
	//////////////////////////////命令

	/// モデル読み込み
	int LoadModel(std::string Path);

public:
	//////////////////////////////エンジン内部命令

	/// Texture指令
	int LoadCommonTexture(const std::string& filePath);
	int LoadModelTexture(const std::string& filePath);

	int GetTextureHandleFromCommonList(int index);
	int GetTextureHandleFromModelGroup(int modelHandle, int part);

	DirectX::TexMetadata GetTextureMetaData(int textureHandle);

	int ReadModelTextureHandle(int index);
	int ReadCommonTextureHandle(int index);

	bool SetModelTexture(Model* model);

	void ResizeSimpleSpriteMesh(DirectX::TexMetadata Metadata, int counter, CornerData corner, Vector2 anchorPoint, Vector2 cropLT, Vector2 cropSize);

	/// 暫くのCounter管理
	int GetTextureCounter();
	void TextureCounterPlus(int index = 1);
	void TextureCounterAdjust(int index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetTextureCPUDescriptorHandle(int handle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureGPUDescriptorHandle(int handle);

	void ResizeSimpleSpriteMeshList(int spriteNumber);
	void DeleteExtraSpriteMesh(int spriteNumber);

	int InputMaterialConfig(std::shared_ptr<MaterialConfig> material);


public:

	/// singletonドライブ
	static ResourceManager* instance_;

	/// 借りのDevice
	DirectXCore* core_ = nullptr;
	ID3D12Device* BDevice_ = nullptr;

	//////////////////////////////Texture関係

	/// Material関係
	std::vector<BasicResource*> materialResourceList_;

	struct MaterialEntry {
		MaterialID materialID{};
		std::weak_ptr<MaterialConfig> config{}; 
		std::unique_ptr<Material> cpuMaterial;  
		BasicResource* gpuMaterial{};           
		int textureHandle{};                    
	};

	std::vector<MaterialEntry> materialList_;
	std::unordered_map<MaterialID, int> idToIndex_;
	int materialCounter_{};

	//////////////////////////////Vertex\Index関係

	/// 図形関係
	std::vector	<MeshBuffer*> meshBufferList_;		/// すべでのモデルを収納するどころ		これを使って解放する
	std::vector	<Sprite2DMesh*> spriteMeshHandles_;	/// スブライドのハンドルを収納する		解放に使えない
	std::vector	<ModelGroup*> modelGroupList_;		/// モデルグループを	収納する			解放に使えない
	std::vector <SimpleSpriteMesh*> simpleSpriteMeshList_;	/// デフォルトのスプライトメッシュ

	/// ModelHandle
	int modelHandleCounter_ = 0;


private:

	/// インストラクター・デストラクター封印
	ResourceManager() = default;
	~ResourceManager() = default;


	/// リソース作り
	int CreateSimpleSpriteMeshResource();
	int CreateTriangleResource();
	int CreateCubeResource();
	int CreateSphereResource(int sudivision);

	int CreateModelResource(std::string Path);

private:
	////////////////////////////// 関数テンプレート

	/// ポインタ解放テンプレート
	template<typename T>
	void ClearPointer(std::vector< T* >& list) {
		for (auto& ptr : list) {
			delete ptr;
			ptr = nullptr;
		}
		list.clear();
	}

	template<typename T>
	bool CheckInstance(std::vector< T* >& list, T target, bool useCustomCheck) {

		auto checker = std::find_if(list.begin(),
			list.end(),
			[&](T* ptr) {return *ptr == target; });

		return checker == list.end();
	}


};

