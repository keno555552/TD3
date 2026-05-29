#pragma once
#include "BasicResource.h"
#include "Mesh/VertexResource.h"
#include "Data/Render/CPUData/VertexData.h"
#include "Data/Render/GPUData/MaterialForGPU.h"
#include "Data/Render/CPUData/MaterialConfig.h"
#include "mesh/VertexIndex.h"
//#include "InstanceManager.h"
#include "TextureManager.h"
#include "config.h"
#include "Data/Render/CPUData/ObjectData.h"
#include "Data/Render/CPUData/SpriteData.h"
#include "Data/Render/CpuData/RenderTexture.h"
#include "DescriptorManager/SrvManager/SrvManager.h"
#include "DescriptorManager/RtvManager/RtvManager.h"
#include "DescriptorManager/DsvManager/DsvManager.h"
#include <memory>
using MaterialID = int;

class ResourceManager
{
public:
	//struct TextureInfo {
	//	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	//	int width;
	//	int height;
	//	Vector4 uvOffset;
	//};

#pragma region Instance管理


#pragma endregion

public:

	/// シングルトン取得
	static ResourceManager* GetInstance();

	class ConstructorKey {
	private:
		/// からのみ生成・破棄可能
		friend class ResourceManager;
		friend class kEngine;
		ConstructorKey() {}
	};

	explicit ResourceManager(ConstructorKey) {};

	void Initialize(DirectXCore* device);
	void Finalize();

	static void Destroy();


	/// ランクこと作成するResource
	void CreateTurnResource();

	/// ランクこと解放するResource
	void ClearTurnResource();

public:
	//////////////////////////////命令

	/// ファイル読み込み
	int ReadFile(std::string Path);

	/// ハンドルからModelDataを取得
	std::shared_ptr<ModelData> GetModelData(int handle);

	/// モデル読み込み
	int LoadModel(std::string Path);

	/// /// エンジン内蔵のモデルを作る
	int CreateEngineModel(RingBuildMaterial& buildMaterial);
	int CreateEngineModel(SphereBuildMaterial& buildMaterial);
	int CreateEngineModel(CylinderBuildMaterial& buildMaterial);

	/// /// エンジン内蔵のモデルを改変する
	void UpdateEngineModel(RingBuildMaterial& buildMaterial, int modelHandle, int meshHandle = -1);
	void UpdateEngineModel(SphereBuildMaterial& buildMaterial, int modelHandle, int meshHandle = -1);
	void UpdateEngineModel(CylinderBuildMaterial& buildMaterial, int modelHandle, int meshHandle = -1);

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


	/// ==================== RenderTexture制作用 ===================== ///

	RenderTexture CreateRenderTexture(
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		const Vector4& clearColor);


	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(
		uint32_t width,
		uint32_t height,
		DXGI_FORMAT format,
		const Vector4& clearColor
	);

	void CreateRTV(ID3D12Resource* renderTexture, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
	void CreateSRV(ID3D12Resource* renderTexture, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);



	/// ===================== DepthStencil関連 ====================== ///
	/// 普通のDepthStencilを作る関数
	ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

	/// RenderTexture専用のDepthStencilを作る関数
	void CreateDepthStencilForRenderTexture(RenderTexture& rt);


	/// ==================== TextureManager関連 ===================== ///

	D3D12_CPU_DESCRIPTOR_HANDLE GetTextureCPUDescriptorHandle(int handle);
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureGPUDescriptorHandle(int handle);

	void ResizeSimpleSpriteMeshList(int spriteNumber);
	void DeleteExtraSpriteMesh(int spriteNumber);

	int InputMaterialConfig(std::shared_ptr<MaterialConfig> material);


public:

	/// singletonドライブ
	static std::unique_ptr<ResourceManager> instance_;

	/// 借りのDevice
	DirectXCore* core_ = nullptr;
	ID3D12Device* BDevice_ = nullptr;

	//////////////////////////////ModelDataList

	std::unordered_map<std::string, std::shared_ptr<ModelData>> modelDataList_;	/// ModelDataを収納するリスト
	std::unordered_map<int, std::string> modelDataHandleMap_; /// ModelDataのハンドルを管理するマップ
	int modelDataCounter_{}; /// ModelDataのハンドルカウンター

	//////////////////////////////Texture関係

	/// Material関係
	std::vector<std::unique_ptr<BasicResource>> materialResourceList_;

	struct MaterialEntry {
		MaterialID materialID{};
		std::weak_ptr<MaterialConfig> config{};
		std::unique_ptr<MaterialForGPU> cpuMaterial;
		BasicResource* gpuMaterial{};
		int textureHandle{};
		int isUsed{}; // 0:未使用 1:使用中 2:解放予定
	};

	std::vector<MaterialEntry> materialList_;
	std::unordered_map<MaterialID, int> idToIndex_;
	int materialCounter_{};

	//////////////////////////////Vertex\Index関係

	// 1.モデルハンドルはModelGroupのIndex。
	// 2.DDCにモデルの取り方はModelGroupから直接とること
	// 3.モデルの解放はmeshBufferListから行う(今はshared_ptrで管理してるから自動的に解放されるはず)
	// 4.ModelGroupの中のmeshHandleのlistはmeshBufferListのIndexを保存する

	/// 図形関係
	std::vector<std::shared_ptr<MeshBuffer>> meshBufferList_;				/// すべでのモデルを収納するどころ		これを使って解放する
	std::vector<std::shared_ptr<ModelGroup>> modelGroupList_;				/// モデルグループを	収納する		解放に使えない
	std::vector<std::shared_ptr<Sprite2DMesh>> spriteMeshHandles_;			/// スブライドのハンドルを収納する		解放に使えない
	std::vector<std::shared_ptr<SimpleSpriteMesh>> simpleSpriteMeshList_;	/// デフォルトのスプライトメッシュ
	std::vector<int> deleteMeshHandleList_;

	/// ModelHandle
	int modelHandleCounter_ = 0;


private:

	friend struct std::default_delete<ResourceManager>;
	~ResourceManager() = default;
private:

	/// リソース作り
	int CreateSimpleSpriteMeshResource();
	int CreateTriangleResource();
	int CreatePlaneResource();
	int CreateCubeResource();
	int CreateSphereResource(int sudivision);
	int CreateSkyCubeResource();
	int CreateRingResource(int subdivision,float OuterRadius, float InnerRadius);
	int CreateCylinderResource(int division, float topRadius, float bottomRadius, float height);

	int CreateModelResource(std::string Path);

	/// リソース解放


	/// リソース切り替え
	void SwapMeshAndModelGroup(std::shared_ptr<Model> model, int modelHandle, int modelIndex = -1);

private:
	////////////////////////////// 関数テンプレート

	/// ポインタ解放テンプレート
	//template<typename T>
	//void ClearPointer(std::vector< T* >& list) {
	//	for (auto& ptr : list) {
	//		delete ptr;
	//		ptr = nullptr;
	//	}
	//	list.clear();
	//}
	//
	//template<typename T>
	//bool CheckInstance(std::vector< T* >& list, T target, bool useCustomCheck) {
	//
	//	auto checker = std::find_if(list.begin(),
	//		list.end(),
	//		[&](T* ptr) {return *ptr == target; });
	//
	//	return checker == list.end();
	//}

	public:
		void CleanupUnusedMaterials();
};


//TODO: MeshBufferを管理するクラスを作る
//TODO: MaterialEntryをInstanceとPSOし、管理する

// 像現在一樣，製作一個MaterialEntry和MaterialInstance
// MaterialEntry用作收藏不宜變化的東西，照現在的邏輯去更新
// 再加上一個一定時間會自動刪除沒在用的單元的邏輯
// MaterialInstance則是用cbuffer形式儲存，在MaterialEntry處以index形式連接
// Instance不需比較，直接更新帶入
// 以做到快速更新