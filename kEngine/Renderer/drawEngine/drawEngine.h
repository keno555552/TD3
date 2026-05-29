#pragma once
#include "DirectXCore.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <memory>
#include "Config.h"
#include "PSOManager/PSOManager.h"
#include "Resource/ResourceManager.h"
#include "Data/Render/CPUData/VertexData.h"
#include "Data/Render/GPUData/MaterialForGPU.h"
#include "GPUData/DirectionalLightGPU.h"

#include "MathsIncluder.h"
#include "TransformationMatrix.h"
#include "StringManage/ConvertString.h"
#include "Data/Render/CPUData/MaterialConfig.h"
#include "mesh/VertexIndex.h"
#include "Camera/Camera.h"
#include "Data/Render/GPUData/CameraForGPU.h"
#include "GPUData/LightGPU.h"

#include "Data/Render/CPUData/ObjectData.h"
#include "Data/Render/CPUData/SpriteData.h"
#include <format>

#include "DescriptorManager/SrvManager/SrvManager.h"
#include "DrawData/DrawDataCollector.h"
#include "Renderer/Resource/InstanceBuffer.h"
#include "Renderer/PostProcessRunner/PostProcessRunner.h"

class DrawEngine
{
public:

	friend class PostProcessRunner;


	void Initialize(
		DirectXCore* directXDirver,
		DrawDataCollector* drawDataCollector,
		PostProcessRunner* postProcessRunner
	);

	void Finalize();

	void StartFrame();
	void PreDraw();
	void CommitDraw();
	void EndDraw();


	/// ======== 全部描く関数 ======== ///
	/// DebugLinee描画関数
	void DrawDebugLine();

	/// 2D描画関数
	void Draw2D();
	void Draw2DTransparent();
	void Draw2DOpaque();

	/// 3D描画関数
	void Draw3D();
	void Draw3DTransparent();
	void Draw3DOpaque();

	/// Particle描画関数
	void DrawParticle();

	/// 描画関数のコア
	void DrawCall();

	/// EnviromentReflection関連関数
	void SetEnviromentReflectionTexture(int textureHandle);

	/// Skinning関連関数
	void CreateSkinningBuffer(ObjectData* objectData);
	void ClearSkinningBuffer(ObjectData* objectData);

	/// リソースローディング
	int GetModelTextureHandle(int modelHandle, int part);

	int readModelTextureHandle(int Handle);
	int readCommonTextureHandle(int Handle);

	int LoadModelTexture(const std::string& filePath);


private:


	std::unique_ptr<PSOManager> psoManager_{};
	PostProcessRunner* postProcessRunner_{};		/*依存*/
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

private:
	/// PSO関連

	LightModelType defaultLightModel_ = LightModelType::Lambert;

private:
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};

	/// Texture関連
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_{};
	uint32_t descriptorIndex_ = 1;						// 0はImGui用に予約
	std::vector<int> commonTextureSRVMap_;
	std::vector<int> modelTextureSRVMap_;
	int defaultTextureHandle_ = 0;						// white5x5
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = nullptr;
	//ID3D12Resource* depthStencilResource = nullptr;			// ResourceManagerで作るから、ここではポインタだけもらう


	///Lighting関連
	//D3D12_GPU_DESCRIPTOR_HANDLE lightListSrvHandleGPU_{};
	//std::unique_ptr <BasicResource> lightBuffer_;

	std::unique_ptr<InstanceBuffer<LightGPU>>lightBuffer_;
	LightGPU* lightListData_ = nullptr;                  // 受け皿

	uint32_t lightCount_ = 0;

	/// カメラ関連
	CameraForGPU* cameraPtr_ = nullptr;                  // 受け皿
	std::unique_ptr <BasicResource> cameraBuffer_;

	/// DebugLine描画関連
	Microsoft::WRL::ComPtr<ID3D12Resource> debugLineVB_;
	D3D12_VERTEX_BUFFER_VIEW debugLineVBView_{};
	size_t debugLineVertexBufferSize_ = 0;

	/// Instance-GPU交換用容器
	std::unique_ptr<InstanceBuffer<TransformationMatrix>> debugLineResource_;
	std::unique_ptr<InstanceBuffer<TransformationMatrix>> tile2DWVPResource_;
	std::unique_ptr<InstanceBuffer<TransformationMatrix>> tile3DWVPResource_;
	std::unique_ptr<InstanceBuffer<TransformationMatrix>> tilePCWVPResource_;
	int instanceDLCounter_ = 0;
	int instance2DCounter_ = 0;
	int instance3DCounter_ = 0;
	int instancePCCounter_ = 0;

	/// EnviromentReflection関連
	std::unique_ptr <BasicResource> enviromentReflectionTexture_;
	int enviromentReflectionTextureHandle_ = -1;

	/// Skinning関連
	std::vector<std::unique_ptr<InstanceBuffer<WellForGPU>>> skinningWFGResourceList_;
	// DDCにのSkinningDataのハンドルと、実際のWellForGPUのマップ
	std::map<int, int> skinningDatDDC2DEaMap_;
	std::vector<int> skinningBufferFreeList_;
	//int skinningCounter_ = 0;

private:

	/// Instance資料指定用のOffsetData構造体とリスト
	struct OffsetData {
		std::unique_ptr <BasicResource> instanceOffsetResource;
		UINT* instanceOffset{};
		int state = 0;// 0:未使用 1:使用中
	};

	std::vector<OffsetData> instanceOffsetData_;
	int offsetDataCounter_{};

private:

	/// PingPong用のRenderTexture
	RenderTexture m_Offscreen_InputRT{};
	RenderTexture m_Offscreen_OutputRT{};

private:
	/// 内部関数
	D3D12_VIEWPORT createViewport(int kClientWidth, int kClientHeight);
	D3D12_RECT createScissorRect(int kClientWidth, int kClientHeight);

	void IntializeInstanceTMBuffer(TransformationMatrix* bufferPointer, size_t count);

	void SetMaterial(int materialID);
	void SetTexture(int materialID);
	void SetCameraForGPU();
	void UpdateLighting();
	void SetLightingGPU();
	void SetEnviromentReflectionGPU();

	void PSODecision(PSOKey& psoKey);
	void MakeDepthStencilView();

	void UpdateDebugLineVertexBuffer(const std::vector<DebugLineVertexGPU>& vertices);

	/// ===== PostProcess描画関数

	void TransitionRenderTarget(
		RenderTexture& renderTexture,
		D3D12_RESOURCE_STATES toState
	);

	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE renderTarget);

	void SetSRVHeap();

	void SetRootDescriptorTable(UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE descriptorHandle);

	/// PostProcess群
	void DrawColorGrading();
	void DrawVignette();
	void DrawBlur();

	/// RenderCopy(描画内容をそのまま描画する、最後の処理)
	void DrawRenderCopy();

	/// Offscreen描画関数
	void DrawFullscreenQuad();

private:

	/// <summary>
	/// InstanceBufferを作成する関数
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="resource"></param>
	/// <param name="count"></param>
	/// <returns></returns>
	template<typename T>
	T* CreateInstanceBuffer(std::unique_ptr<BasicResource>& resource, size_t count) {
		T* cpuPtr = nullptr;

		resource = std::make_unique<BasicResource>();
		resource->CreateResourceClass_(
			directXDriver_->GetDevice(),
			sizeof(T) * count
		);

		resource->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&cpuPtr));
		return cpuPtr;
	}

private:
	bool isFinish = false;
};

//TODO: DrawEngineは移動したが、それに関わる対応がしてない
// 今のは本当のInstance描きではない
// 本当のInstance描きはmeshを融合しないと呼べない
// 今はただ分類して描いてるだけ