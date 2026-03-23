#pragma once
#include "DirectXCore.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <memory>
#include "Config.h"
#include "PSOManager/PSOFactory.h"
#include "Resource/ResourceManager.h"
#include "Data/Render/CPUData/VertexData.h"
#include "Data/Render/GPUData/MaterialForGPU.h"
#include "GPUData/DirectionalLightGPU.h"

#include "MathsIncluder.h"
#include "TransformationMatrix.h"
#include "StringManage/ConvertString.h"
#include "Data/Render/Types/LightModelType.h"
#include "Data/Render/CPUData/MaterialConfig.h"
#include "mesh/VertexIndex.h"
#include "Camera/Camera.h"
#include "Data/Render/GPUData/CameraForGPU.h"
#include "GPUData/LightGPU.h"

#include "Data/Render/CPUData/ObjectData.h"
#include "Data/Render/CPUData/SpriteData.h"
#include <format>

#include "SrvManager/SrvManager.h"
#include "DrawData/DrawDataCollector.h"

class DrawEngine
{
public:
	void Initialize(DirectXCore* directXDirver,
					SrvManager* srvManager, 
					ResourceManager* resourceManager,
					DrawDataCollector* drawDataCollector);
	
	void Finalize();

	void StartFrame();
	void PreDraw();
	void CommitDraw();
	void EndDraw();


	/// ======== 全部描く関数 ======== ///
	/// 2D描画関数
	void Draw2D();
	void Draw2DTransparent();
	void Draw2DOpaque();

	/// 3D描画関数
	void Draw3D();
	void Draw3DTransparent();
	void Draw3DOpaque();

	void DrawCall();

	/// リソースローディング
	int GetModelTextureHandle(int modelHandle, int part);

	int readModelTextureHandle(int Handle);
	int readCommonTextureHandle(int Handle);

	int LoadModelTexture(const std::string& filePath);


private:
	
	std::unique_ptr<PSOFactory> pso_ = std::make_unique<PSOFactory>();
	ResourceManager* resourceManager_{};			/*依存*/
	DirectXCore* directXDriver_{};					/*依存*/
	ID3D12GraphicsCommandList* commandList_{};		/*依存*/
	SrvManager* srvManager_{};						/*依存*/
	DrawDataCollector* drawDataCollector_{};		/*依存*/

	int kClientWidth_ = 0;
	int kClientHeight_ = 0;

	int kMaxSubdivision_ = 18;
	int kSubdivision_ = 0;

private:
	enum class psoType {
		NONE = -1,
		defaultPSO = 1,
		Sprite2D = 0,
		Lambert,
		HalfLambert,
		PhongReflection,
		BlinnPhongReflection,
	};

private:
	/// PSO関連

	LightModelType defaultLightModel_ = LightModelType::Lambert;
	psoType currentPSO_ = psoType::NONE;

private:
	std::vector <Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoList_;
	ID3D12RootSignature* rootSignature_ = nullptr; 			// Listからもセーブしたから解放しなくていい	
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};

	/// Texture関連
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE Tile2DSrvHandleGPU_{};
	D3D12_GPU_DESCRIPTOR_HANDLE Tile3DSrvHandleGPU_{};
	uint32_t descriptorIndex_ = 1;						// 0はImGui用に予約
	std::vector<int> commonTextureSRVMap_;
	std::vector<int> modelTextureSRVMap_;
	int defaultTextureHandle_ = 0;						// white5x5
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;
	//ID3D12Resource* depthStencilResource = nullptr;			// ResourceManagerで作るから、ここではポインタだけもらう


	///Lighting関連
	D3D12_GPU_DESCRIPTOR_HANDLE lightListSrvHandleGPU_{};
	std::unique_ptr <BasicResource> lightBuffer_;
	LightGPU* lightListData_ = nullptr;                  // 受け皿

	uint32_t lightCount_ = 0;


	/// Instance-GPU交換用容器
	std::unique_ptr <BasicResource> tile2DWVPResource_;
	std::unique_ptr <BasicResource> tile3DWVPResource_;
	int instance2DCounter_ = 0;
	int instance3DCounter_ = 0;

	/// カメラ関連
	CameraForGPU* cameraPtr_ = nullptr;                  // 受け皿
	std::unique_ptr <BasicResource> cameraBuffer_;

private:

	struct OffsetData {
		std::unique_ptr <BasicResource> instanceOffsetResource;
		UINT* instanceOffset{};
		int state = 0;// 0:未使用 1:使用中
	};

	std::vector<OffsetData> instanceOffsetData_;
	int offsetDataCounter_{};


private:
	/// 内部関数
	D3D12_VIEWPORT createViewport(int kClientWidth, int kClientHeight);
	D3D12_RECT createScissorRect(int kClientWidth, int kClientHeight);
	void SetMaterial(int materialID);
	void SetTexture(int materialID);
	void SetCameraForGPU();
	void InitializeLighting();
	void UpdateLighting();
	void SetLightingGPU();

	void PSODecision(MaterialConfig& material);
	void PSODecision(int psoID);
	ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);
	void MakeDepthStencilView();

private:
	bool isFinish = false;
};

//TODO: DrawEngineは移動したが、それに関わる対応がしてない