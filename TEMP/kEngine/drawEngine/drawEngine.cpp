#include "drawEngine.h"
#include "Transform.h"
#include <math.h>
#include <tool/mathsUnit/Vector3.h>
#define M_PI 3.1415926f
#include "Logger.h"

#include "Queue/RenderData.h"

void DrawEngine::Initialize
(DirectXCore* directXDriver, SrvManager* srvManager, ResourceManager* resourceManager, DrawDataCollector* drawDataCollector) {
	directXDriver_ = directXDriver;
	commandList_ = directXDriver_->GetCommandList();
	srvManager_ = srvManager;
	resourceManager_ = resourceManager;
	drawDataCollector_ = drawDataCollector;
	///
	kClientWidth_ = config::GetClientWidth();
	kClientHeight_ = config::GetClientHeight();

	///
	pso_->Initialize(directXDriver_);

	///================== PSO関連 ==================
	/// PSOつくり
	for (int i = 0; i < (int)LightModelType::NumLightModels; i++) {
		ID3D12PipelineState* graphicsPipelineState_ = pso_->createPSO((LightModelType)i);
		psoList_.push_back(graphicsPipelineState_);
	}

	rootSignature_ = pso_->getRootSignature((int)psoType::defaultPSO);

	depthStencilResource = CreateDepthStencilTextureResource(directXDriver_->GetDevice(), kClientWidth_, kClientHeight_);
	MakeDepthStencilView();

	///
	viewport = createViewport(kClientWidth_, kClientHeight_);
	scissorRect = createScissorRect(kClientWidth_, kClientHeight_);



	///Lighting
	InitializeLighting();

	/// =========================== Tile用wvpBufferを作成 =========================== ///
	///2Dタイル用WVPバッファ
	TransformationMatrix* instanceListPtr2D = nullptr;
	tile2DWVPResource_->CreateResourceClass_(directXDriver_->GetDevice(), sizeof(TransformationMatrix) * config::Get2DTileNumInstance());
	tile2DWVPResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&instanceListPtr2D));
	drawDataCollector_->SetInstanceList2D(instanceListPtr2D);
	
	for (int index = 0; index < config::Get2DTileNumInstance(); ++index) {
		instanceListPtr2D[index].WVP = Identity();
		instanceListPtr2D[index].world = Identity();
		instanceListPtr2D[index].WorldInverseTranspose = Identity();
	}
	int srvHandleIndex = srvManager_->Allocate();
	srvManager_->CreateSRVForStructuredBuffer(srvHandleIndex, tile2DWVPResource_->GetResource().Get(), config::Get2DTileNumInstance(), sizeof(TransformationMatrix));
	Tile2DSrvHandleGPU_ = srvManager_->GetGPUDescriptorHandle(srvHandleIndex);

	///3Dタイル用WVPバッファ
	TransformationMatrix* instanceListPtr3D = nullptr;
	tile3DWVPResource_->CreateResourceClass_(directXDriver_->GetDevice(), sizeof(TransformationMatrix) * config::Get3DTileNumInstance());
	tile3DWVPResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&instanceListPtr3D));
	drawDataCollector_->SetInstanceList3D(instanceListPtr3D);

	for (int index = 0; index < config::Get3DTileNumInstance(); ++index) {
		instanceListPtr3D[index].WVP = Identity();
		instanceListPtr3D[index].world = Identity();
		instanceListPtr3D[index].WorldInverseTranspose = Identity();
	}
	srvHandleIndex = srvManager_->Allocate();
	srvManager_->CreateSRVForStructuredBuffer(srvHandleIndex, tile3DWVPResource_->GetResource().Get(), config::Get3DTileNumInstance(), sizeof(TransformationMatrix));
	Tile3DSrvHandleGPU_ = srvManager_->GetGPUDescriptorHandle(srvHandleIndex);

	/// ====================== InstanceOffset用バッファを作成 ======================= ///
	for (int i = 0; i < config::GetMaxMaterialNum(); i++) {
		OffsetData* offsetData = new OffsetData;
		UINT* offset = nullptr;
		offsetData->instanceOffsetResource->CreateResourceClass_(directXDriver_->GetDevice(), 256);
		offsetData->instanceOffsetResource->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&offset));
		offsetData->instanceOffset = offset;
		offsetData->state = 0;
		instanceOffsetData_.push_back(offsetData);
	}

	/// =========================== カメラバッファの初期化 =========================== ///
	cameraBuffer_->CreateResourceClass_(directXDriver_->GetDevice(), sizeof(CameraForGPU));
	cameraBuffer_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&cameraPtr_));


	/// ========================== デフォルトのモデルを設定 ========================== ///
	config::default_Plane_MeshBufferHandle_ = resourceManager_->LoadModel("resources/TemplateResource/object/plane/plane.obj");
	defaultTextureHandle_ = resourceManager_->LoadModelTexture("resources/TemplateResource/texture/white5x5.png");

}

void DrawEngine::Finalize() {
	for (auto& ptr : psoList_) {
		ptr->Release();
		ptr = nullptr;
	}
	psoList_.clear();

	delete pso_;

	delete tile2DWVPResource_;
	delete tile3DWVPResource_;

	delete cameraBuffer_;

	for (auto& ptr : instanceOffsetData_) {
		delete ptr->instanceOffsetResource;
		ptr->instanceOffsetResource = nullptr;
		ptr->instanceOffset = nullptr;
		delete ptr;
		ptr = nullptr;
	}

	if (depthStencilResource) {
		depthStencilResource->Release();
		depthStencilResource = nullptr;
	}
}

void DrawEngine::StartFrame() {
	// 描画用のDescriptorHeapの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvManager_->GetDescriptorHeap() };  // 現在使用正確的 getter 函式
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	commandList_->RSSetViewports(1, &viewport);  // Viewportを設定
	commandList_->RSSetScissorRects(1, &scissorRect);  // Scissorを設定

	/// 各種のリソースを設定(今内容がない)
	resourceManager_->CreateTurnResource();


	/// InstanceCounterReset
	instance2DCounter_ = 0;
	instance3DCounter_ = 0;
	offsetDataCounter_ = 0;
}

void DrawEngine::PreDraw() {

	/// 形状を設定。PSOに設定しているものとはまた別。同じものを設定するとUpdateLighting考えておけば良い
	commandList_->SetGraphicsRootSignature(rootSignature_);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	currentPSO_ = psoType::NONE;

	/// Lighting
	drawDataCollector_->UpdateLightData();
	UpdateLighting();

	/// Set Camera
	SetCameraForGPU();

}

void DrawEngine::CommitDraw() {


	/// 集まったデータで描画
	/// Sprite描画
	DrawCall();
}

void DrawEngine::EndDraw() {

	/// 各種のリソースを解放
	resourceManager_->ClearTurnResource();

	for (auto& ptr : instanceOffsetData_) {
		if (ptr->state == 1) { ptr->state = 2; }
		if (ptr->state == 2) { ptr->state = 0; }
	}
}


void DrawEngine::PSODecision(MaterialConfig& material) {
	bool psoChanged = false;

	LightModelType lightModelType = (LightModelType)(int)material.lightModelType;

	switch (lightModelType) {
	case LightModelType::Sprite2D:
		if (currentPSO_ != psoType::Sprite2D) {
			commandList_->SetPipelineState(psoList_[(int)LightModelType::Sprite2D]);
			currentPSO_ = psoType::Sprite2D;
			psoChanged = true;
		}
		break;
	case LightModelType::Lambert:
		if (currentPSO_ != psoType::Lambert) {
			commandList_->SetPipelineState(psoList_[(int)LightModelType::Lambert]);
			currentPSO_ = psoType::Lambert;
			psoChanged = true;
		}
		break;
	case LightModelType::HalfLambert:
		if (currentPSO_ != psoType::HalfLambert) {
			commandList_->SetPipelineState(psoList_[(int)LightModelType::HalfLambert]);
			currentPSO_ = psoType::HalfLambert;
			psoChanged = true;
		}
		break;
	case LightModelType::PhongReflection:
		if (currentPSO_ != psoType::PhongReflection) {
			commandList_->SetPipelineState(psoList_[(int)LightModelType::PhongReflection]);
			currentPSO_ = psoType::PhongReflection;
			psoChanged = true;
		}
	case LightModelType::BlinnPhongReflection:
		if (currentPSO_ != psoType::BlinnPhongReflection) {
			commandList_->SetPipelineState(psoList_[(int)LightModelType::BlinnPhongReflection]);
			currentPSO_ = psoType::BlinnPhongReflection;
			psoChanged = true;
		}
	}
	if (psoChanged) {
		rootSignature_ = pso_->getRootSignature((int)currentPSO_);
		commandList_->SetGraphicsRootSignature(rootSignature_);
	}
}

void DrawEngine::PSODecision(int psoID) {
	auto newPSO = static_cast<psoType>(psoID);
	if (currentPSO_ != newPSO) {
		commandList_->SetPipelineState(psoList_[psoID]);
		currentPSO_ = newPSO;

		rootSignature_ = pso_->getRootSignature((int)currentPSO_);
		commandList_->SetGraphicsRootSignature(rootSignature_);
	}
}

void DrawEngine::Draw2D() {

	if (drawDataCollector_->GetOpaqueBuckets2D().empty() &&
		drawDataCollector_->GetTransparentObjectParts2D().empty())return;

	Draw2DOpaque();
	Draw2DTransparent();

}

void DrawEngine::Draw2DTransparent() {

	auto& transparent2D_ = drawDataCollector_->GetTransparentObjectParts2D();

	/// TileSRV
	commandList_->SetGraphicsRootDescriptorTable(1, Tile2DSrvHandleGPU_);


	for (auto& object : transparent2D_) {

		/// Set PSO
		PSODecision((int)object.psoID);

		SetLightingGPU();

		SetMaterial(object.materialID);

		SetTexture(object.materialID);

		/// MeshIndex 數量
		int meshIndexCount = object.mesh->GetIndexNum();

		/// VBV/IBV 設定
		D3D12_VERTEX_BUFFER_VIEW vbv = object.mesh->GetVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = object.mesh->GetIndexBufferView();
		commandList_->IASetVertexBuffers(0, 1, &vbv);
		commandList_->IASetIndexBuffer(&ibv);



		/// WVP 設定
		int instIdx = instance2DCounter_;
		++instance2DCounter_;

		/// OffsetData 設定
		OffsetData* inUse = instanceOffsetData_[offsetDataCounter_];
		*inUse->instanceOffset = static_cast<UINT>(instIdx);
		inUse->state = 1;
		commandList_->SetGraphicsRootConstantBufferView(4, inUse->instanceOffsetResource->GetResource()->GetGPUVirtualAddress());

		/// Draw
		if (meshIndexCount != 0) {
			commandList_->DrawIndexedInstanced(meshIndexCount, 1, 0, 0, 0);
		} else {
			int meshVertexCount = object.mesh->GetVertexNum();
			commandList_->DrawInstanced(meshVertexCount, 1, 0, 0);
		}

		++offsetDataCounter_;
	}
}

void DrawEngine::Draw2DOpaque() {
	auto& transparentObjectParts2D_ = drawDataCollector_->GetOpaqueBuckets2D();

	/// TileSRV
	commandList_->SetGraphicsRootDescriptorTable(1, Tile2DSrvHandleGPU_);

	for (auto& [psoID, materialBuckets] : transparentObjectParts2D_) {

		/// Set PSO
		PSODecision((int)psoID);

		for (auto& [materialID, RenderDataGroup] : materialBuckets) {

			if (RenderDataGroup.empty()) continue;

			SetLightingGPU();

			SetMaterial(materialID);

			SetTexture(materialID);

			/// 不透明ものの描画

			for (auto& [meshBuffer, RenderData] : RenderDataGroup) {

				/// Instancing 用のデータを準備
				int instancesCounter = 0;

				/// MeshIndex 數量
				int meshIndexCount = RenderData[0].mesh->GetIndexNum();

				/// VBV/IBV 設定
				D3D12_VERTEX_BUFFER_VIEW vbv = RenderData[0].mesh->GetVertexBufferView();
				D3D12_INDEX_BUFFER_VIEW ibv = RenderData[0].mesh->GetIndexBufferView();
				commandList_->IASetVertexBuffers(0, 1, &vbv);
				commandList_->IASetIndexBuffer(&ibv);

				/// インスタンスの開始位置を保存
				int instIdx = instance2DCounter_;

				/// WVP計算
				for (auto& object : RenderData) {
					instancesCounter++;
					instance2DCounter_++;
				}

				/// 設定 offset
				OffsetData* inUse = instanceOffsetData_[offsetDataCounter_];
				*inUse->instanceOffset = static_cast<UINT>(instIdx);
				inUse->state = 1;
				commandList_->SetGraphicsRootConstantBufferView(4, inUse->instanceOffsetResource->GetResource()->GetGPUVirtualAddress());

				++offsetDataCounter_;


				if (meshIndexCount != 0) {
					commandList_->DrawIndexedInstanced(meshIndexCount, instancesCounter, 0, 0, 0);
				} else {
					int meshVertexCount = RenderData[0].mesh->GetVertexNum();
					commandList_->DrawInstanced(meshVertexCount, instancesCounter, 0, 0);
				}

				//Logger::Log("Draw3D: pso=%d mat=%u meshID=%d instances=%d",(int)psoID, materialID, meshBuffer, instancesCounter);
			}
		}
	}
}

/// ===========================================================================!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/// ===========================================================================!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/// ===========================================================================!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void DrawEngine::Draw3D() {

	if (drawDataCollector_->GetOpaqueBuckets3D().empty() &&
		drawDataCollector_->GetTransparentObjectParts3D().empty())return;

	Draw3DOpaque();
	Draw3DTransparent();

}

void DrawEngine::Draw3DTransparent() {

	auto& transparent3D_ = drawDataCollector_->GetTransparentObjectParts3D();

	/// TileSRV
	commandList_->SetGraphicsRootDescriptorTable(1, Tile3DSrvHandleGPU_);

	for (auto& object : transparent3D_) {

		/// Set PSO
		PSODecision((int)object.psoID);

		SetLightingGPU();

		SetMaterial(object.materialID);

		SetTexture(object.materialID);

		/// MeshIndex 數量
		int meshIndexCount = object.mesh->GetIndexNum();

		/// VBV/IBV 設定
		D3D12_VERTEX_BUFFER_VIEW vbv = object.mesh->GetVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = object.mesh->GetIndexBufferView();
		commandList_->IASetVertexBuffers(0, 1, &vbv);
		commandList_->IASetIndexBuffer(&ibv);

		/// WVP 設定
		int instIdx = instance3DCounter_;
		++instance3DCounter_;

		/// OffsetData 設定
		OffsetData* inUse = instanceOffsetData_[offsetDataCounter_];
		*inUse->instanceOffset = static_cast<UINT>(instIdx);
		inUse->state = 1;
		commandList_->SetGraphicsRootConstantBufferView(4, inUse->instanceOffsetResource->GetResource()->GetGPUVirtualAddress());

		/// Draw
		if (meshIndexCount != 0) {
			commandList_->DrawIndexedInstanced(meshIndexCount, 1, 0, 0, 0);
		} else {
			int meshVertexCount = object.mesh->GetVertexNum();
			commandList_->DrawInstanced(meshVertexCount, 1, 0, 0);
		}

		++offsetDataCounter_;
	}
}

void DrawEngine::Draw3DOpaque() {
	auto& transparentObjectParts3D_ = drawDataCollector_->GetOpaqueBuckets3D();

	/// TileSRV
	commandList_->SetGraphicsRootDescriptorTable(1, Tile3DSrvHandleGPU_);

	for (auto& [psoID, materialBuckets] : transparentObjectParts3D_) {

		/// Set PSO
		PSODecision((int)psoID);


		for (auto& [materialID, RenderDataGroup] : materialBuckets) {

			if (RenderDataGroup.empty()) continue;

			SetLightingGPU();

			SetMaterial(materialID);

			SetTexture(materialID);

			/// 不透明ものの描画

			for (auto& [meshBuffer, RenderData] : RenderDataGroup) {

				/// Instancing 用のデータを準備
				int instancesCounter = 0;

				/// MeshIndex 數量
				int meshIndexCount = RenderData[0].mesh->GetIndexNum();

				/// VBV/IBV 設定
				D3D12_VERTEX_BUFFER_VIEW vbv = RenderData[0].mesh->GetVertexBufferView();
				D3D12_INDEX_BUFFER_VIEW ibv = RenderData[0].mesh->GetIndexBufferView();
				commandList_->IASetVertexBuffers(0, 1, &vbv);
				commandList_->IASetIndexBuffer(&ibv);

				/// インスタンスの開始位置を保存
				int instIdx = instance3DCounter_;

				/// WVP計算
				for (auto& object : RenderData) {
					instancesCounter++;
					instance3DCounter_++;
				}

				/// 設定 offset
				OffsetData* inUse = instanceOffsetData_[offsetDataCounter_];
				*inUse->instanceOffset = static_cast<UINT>(instIdx);
				inUse->state = 1;
				commandList_->SetGraphicsRootConstantBufferView(4, inUse->instanceOffsetResource->GetResource()->GetGPUVirtualAddress());

				++offsetDataCounter_;


				if (meshIndexCount != 0) {
					commandList_->DrawIndexedInstanced(meshIndexCount, instancesCounter, 0, 0, 0);
				} else {
					int meshVertexCount = RenderData[0].mesh->GetVertexNum();
					commandList_->DrawInstanced(meshVertexCount, instancesCounter, 0, 0);
				}
				//Logger::Log("Draw3D: pso=%d mat=%u meshID=%d instances=%d",(int)psoID, materialID, meshBuffer, instancesCounter);
			}
		}
	}
}


void DrawEngine::DrawCall() {
	Draw3D();
	Draw2D();
}

int DrawEngine::readCommonTextureHandle(int handle) {
	return resourceManager_->GetTextureHandleFromCommonList(handle);
}

int DrawEngine::GetModelTextureHandle(int modelHandle, int part) {
	return resourceManager_->GetTextureHandleFromModelGroup(modelHandle, part);
}

int DrawEngine::readModelTextureHandle(int handle) {
	return resourceManager_->ReadModelTextureHandle(handle);
}

int DrawEngine::LoadModelTexture(const std::string& filePath) {
	return resourceManager_->LoadCommonTexture(filePath);
}

D3D12_VIEWPORT DrawEngine::createViewport(int kClientWidth, int kClientHeight) {
	viewport.Width = (float)kClientWidth;
	viewport.Height = (float)kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	return viewport;
}

D3D12_RECT DrawEngine::createScissorRect(int kClientWidth, int kClientHeight) {
	// シザー矩形
	//D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;

	return scissorRect;
}

void DrawEngine::SetMaterial(int materialID) {

	auto checker = resourceManager_->idToIndex_.find(materialID);

	if (checker == resourceManager_->idToIndex_.end()) {
		Logger::Log("[kError]DE:MaterialID not found in ResourceManager!");
		return;
	}

	commandList_->SetGraphicsRootConstantBufferView(0, resourceManager_->materialList_[checker->second].gpuMaterial->GetResource()->GetGPUVirtualAddress());
}

void DrawEngine::SetTexture(int materialID) {

	auto checker = resourceManager_->idToIndex_.find(materialID);

	if (checker == resourceManager_->idToIndex_.end()) {
		Logger::Log("[kError]DE:SetTexture not found in ResourceManager!");
		return;
	}

	int textureHandle = resourceManager_->materialList_[checker->second].textureHandle;
	textureSrvHandleGPU_ = resourceManager_->GetTextureGPUDescriptorHandle(textureHandle);
	commandList_->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_);
}

void DrawEngine::SetCameraForGPU() {

	cameraPtr_->worldPosition = drawDataCollector_->GetCameraPosition();

	commandList_->SetGraphicsRootConstantBufferView(5, cameraBuffer_->GetResource()->GetGPUVirtualAddress());
}

void DrawEngine::InitializeLighting() {

	// マテリアルにデータを書き込む
	//resourceManager_->lightingResource_->CreateResourceClass_(directXDriver_->GetDevice(), sizeof(DirectionalLightGPU));
	//resourceManager_->lightingResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&lightingData));

	/// LightListGPU 作成
	lightBuffer_->CreateResourceClass_(directXDriver_->GetDevice(), sizeof(LightGPU) * config::GetMaxLightNum());
	lightBuffer_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&lightListData_));
	/// 時間あれば初期化をここでやる
	int srvHandleIndex = srvManager_->Allocate();
	srvManager_->CreateSRVForStructuredBuffer(srvHandleIndex, lightBuffer_->GetResource().Get(), config::GetMaxLightNum(), sizeof(LightGPU));

	lightListSrvHandleGPU_ = srvManager_->GetGPUDescriptorHandle(srvHandleIndex);
}

void DrawEngine::UpdateLighting() {

	// Lightingにデータを書き込む
	lightCount_ = drawDataCollector_->GetLightCount();

	for (int i = 0; i < (int)lightCount_; ++i) {
		lightListData_[i] = drawDataCollector_->GetLightGPUBuffer()[i];
	}

}

void DrawEngine::SetLightingGPU() {

	// LightListGPU Set
	commandList_->SetGraphicsRootDescriptorTable(6, lightListSrvHandleGPU_);
	// LightingCount Set
	commandList_->SetGraphicsRoot32BitConstants(3, 1, &lightCount_, 0);
}


ID3D12Resource* DrawEngine::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {

	/// 1.生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;                                      // Textureの幅
	resourceDesc.Height = height;									 // Textureの高さ
	resourceDesc.MipLevels = 1;										 // mipmapの数
	resourceDesc.DepthOrArraySize = 1;								 // 奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			 // DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;								 // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	 // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	 // DepthStencilとして使う通知


	/// 2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// VRAM上に作る

	/// EX1.深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; //1.0f (最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;// フォーマット。Resourceと合わせる

	/// 3.Resourceを生成する
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,					// Heapの設定
		D3D12_HEAP_FLAG_NONE,				// Heapの特殊な設定。特になし。
		&resourceDesc,						// Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 深度値を書き込む状態にしてお
		&depthClearValue,					// Clear最適値
		IID_PPV_ARGS(&resource));			// 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	char buffer[128];
	sprintf_s(buffer, "Create resource at %p\n", resource);
	OutputDebugStringA(buffer);

	return resource;
}

void DrawEngine::MakeDepthStencilView() {
	///DSV構築
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	// DSVHeap先頭にDSVをつくる
	directXDriver_->GetDevice()->CreateDepthStencilView(
		depthStencilResource, &dsvDesc, directXDriver_->GetDsvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart()
	);
}



