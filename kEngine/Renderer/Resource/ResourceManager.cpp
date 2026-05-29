#include "ResourceManager.h"
#include <Object/Object.h>
#include "Importer/Loader/LoadModel.h"


std::unique_ptr <ResourceManager> ResourceManager::instance_ = nullptr;

ResourceManager* ResourceManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = std::make_unique<ResourceManager>(ConstructorKey{});
		//instance_.reset(new ResourceManager);
	}
	return instance_.get();
}

void ResourceManager::Initialize(DirectXCore* device) {

	core_ = device;
	BDevice_ = core_->GetDevice();

	config::default_Plane_MeshBufferHandle_		= CreatePlaneResource();
	config::default_Sprite2D_MeshBufferHandle_	= CreateSimpleSpriteMeshResource();
	config::default_Triangle_MeshBufferHandle_	= CreateTriangleResource();
	config::default_Cube_MeshBufferHandle_		= CreateCubeResource();
	config::default_Sphere_MeshBufferHandle_	= CreateSphereResource(10);
	config::default_SkyCube_MeshBufferHandle_	= CreateSkyCubeResource();
	config::default_Ring_MeshBufferHandle_		= CreateRingResource(32, 1.0f, 0.5f);
	config::default_Cylinder_MeshBufferHandle_	= CreateCylinderResource(32, 1.0f, 1.0f, 3.0f);

}

void ResourceManager::Finalize() {

	materialResourceList_.clear();

	meshBufferList_.clear();
	spriteMeshHandles_.clear();
	modelGroupList_.clear();
	simpleSpriteMeshList_.clear();

	materialList_.clear();
	idToIndex_.clear();
	materialCounter_ = 0;

	modelGroupList_.clear();
	meshBufferList_.clear();
}

void ResourceManager::Destroy() {
	instance_.reset();
}

void ResourceManager::CreateTurnResource() {}


void ResourceManager::ClearTurnResource() {
	TextureManager::GetInstance()->EndUploadingTexture();
}

int ResourceManager::CreateSimpleSpriteMeshResource() {

	std::shared_ptr <SimpleSpriteMesh> newSprite2D_;
	newSprite2D_ = std::make_shared<SimpleSpriteMesh>();
	newSprite2D_->CreateVertexResource_(BDevice_);
	newSprite2D_->CreateVertexBufferView_(4);
	newSprite2D_->CreateIndexResource_(BDevice_);
	newSprite2D_->CreateIndexBufferView_(6);
	newSprite2D_->SetKeep(true);
	simpleSpriteMeshList_.push_back(newSprite2D_);

	return (int)meshBufferList_.size() - 1;
}


int ResourceManager::CreateTriangleResource() {
	std::shared_ptr<TriangleMesh> newTriangle;
	newTriangle = std::make_shared<TriangleMesh>();
	newTriangle->CreateVertexResource_(BDevice_);
	newTriangle->CreateVertexBufferView_(6);
	meshBufferList_.push_back(newTriangle);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newTriangle);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::CreatePlaneResource() {
	std::shared_ptr<PlaneMesh> newPlane;
	newPlane = std::make_shared<PlaneMesh>();
	newPlane->CreateVertexResource_(BDevice_);
	newPlane->CreateVertexBufferView_(4);
	newPlane->CreateIndexResource_(BDevice_);
	newPlane->CreateIndexBufferView_(6);
	meshBufferList_.push_back(newPlane);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newPlane);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::CreateCubeResource() {

	std::shared_ptr <CubeMesh> newCube_;
	newCube_ = std::make_shared<CubeMesh>();
	newCube_->CreateVertexResource_(BDevice_);
	newCube_->CreateVertexBufferView_(24);
	newCube_->CreateIndexResource_(BDevice_);
	newCube_->CreateIndexBufferView_(36);
	meshBufferList_.push_back(newCube_);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newCube_);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::CreateSphereResource(int sudivision) {
	sudivision;

	std::shared_ptr <SphereMesh> newSphere;
	newSphere = std::make_shared<SphereMesh>();
	newSphere->CreateVertexResource_(BDevice_);
	newSphere->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newSphere);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newSphere);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;

}

int ResourceManager::CreateSkyCubeResource() {

	std::shared_ptr <SkyCube> newSkyCube;
	newSkyCube = std::make_shared<SkyCube>();
	newSkyCube->CreateVertexResource_(BDevice_);
	newSkyCube->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newSkyCube);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newSkyCube);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;

}

int ResourceManager::CreateRingResource(int subdivision, float OuterRadius, float InnerRadius) {

	std::shared_ptr <RingMesh> newRing;
	newRing = std::make_shared<RingMesh>();
	newRing->SetRingDivide(subdivision);
	newRing->SetOuterRadius(OuterRadius);
	newRing->SetInnerRadius(InnerRadius);
	newRing->CreateVertexResource_(BDevice_);
	newRing->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newRing);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newRing);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::CreateCylinderResource(int division, float topRadius, float bottomRadius, float height) {

	std::shared_ptr <CylinderMesh> cylinderMesh;
	cylinderMesh = std::make_shared<CylinderMesh>();
	cylinderMesh->SetCylinderDivide(division);
	cylinderMesh->SetTopRadius(topRadius);
	cylinderMesh->SetBottomRadius(bottomRadius);
	cylinderMesh->SetHeight(height);
	cylinderMesh->CreateVertexResource_(BDevice_);
	//cylinderMesh->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(cylinderMesh);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(cylinderMesh);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}



int ResourceManager::CreateEngineModel(RingBuildMaterial& buildMaterial) {

	std::shared_ptr <RingMesh> newRing;
	newRing = std::make_shared<RingMesh>();

	if (buildMaterial.Subdivision != 0) newRing->SetRingDivide(buildMaterial.Subdivision);
	if (buildMaterial.OuterRadius != 0) newRing->SetOuterRadius(buildMaterial.OuterRadius);
	if (buildMaterial.InnerRadius != 0) newRing->SetInnerRadius(buildMaterial.InnerRadius);

	newRing->CreateVertexResource_(BDevice_);
	newRing->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newRing);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newRing);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}


int ResourceManager::CreateEngineModel(SphereBuildMaterial& buildMaterial) {

	std::shared_ptr <SphereMesh> newSphere;
	newSphere = std::make_shared<SphereMesh>();

	if (buildMaterial.LatitudeSegments != 0)	newSphere->SetLat(buildMaterial.LatitudeSegments);
	if (buildMaterial.LongitudeSegments != 0)	newSphere->SetLong(buildMaterial.LongitudeSegments);

	newSphere->CreateVertexResource_(BDevice_);
	newSphere->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newSphere);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newSphere);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::CreateEngineModel(CylinderBuildMaterial& buildMaterial) {

	std::shared_ptr <CylinderMesh> newCylinder;
	newCylinder = std::make_shared<CylinderMesh>();

	if (buildMaterial.Division != 0)	newCylinder->SetCylinderDivide(buildMaterial.Division);
	if (buildMaterial.TopRadius != 0)	newCylinder->SetTopRadius(buildMaterial.TopRadius);
	if (buildMaterial.BottomRadius != 0)	newCylinder->SetBottomRadius(buildMaterial.BottomRadius);
	if (buildMaterial.Height != 0)	newCylinder->SetHeight(buildMaterial.Height);
	newCylinder->SetReverseY(buildMaterial.isReverseY);

	newCylinder->CreateVertexResource_(BDevice_);
	newCylinder->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newCylinder);

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->PushModel(newCylinder);
	modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::CreateModelResource(std::string Path) {

	/// パースによって同じモデルがリストにいるかを確認
	if (!modelGroupList_.empty()) {
		for (int i = 0; i < (int)modelGroupList_.size(); i++) {
			std::string checkPath = modelGroupList_[i]->GetFilePath();
			if (checkPath == Path) {
				return i;
			}
		}
	}

	/// ファイルを読み取る,或は読んだファイルのハンドルを探す
	int modelDataHandle = ReadFile(Path);

	/// ModelDataを準備
	std::shared_ptr<ModelData> modelDataPointer = modelDataList_[Path];

	/// ないからモデルグループを作成
	auto modelGroup = std::make_shared<ModelGroup>();

	/// ModelDataからModelを作成してModelGroupに追加
	for (int i = 0; i < modelDataPointer.get()->meshDataList.size(); i++) {

		auto newModel = std::make_shared<Model>();

		newModel->SetModelData(modelDataPointer, i);
		newModel->CreateVertexResource_(BDevice_);
		newModel->CreateIndexResource_(BDevice_);

		modelGroup->PushModel(newModel);

		meshBufferList_.push_back(newModel);
		modelGroup->PushMeshHandle((int)meshBufferList_.size() - 1);
	}

	/// ModelGroupをModelGroupListに追加
	modelGroupList_.push_back(modelGroup);

	/// ハンドルを返す
	return (int)modelGroupList_.size() - 1;
}

void ResourceManager::UpdateEngineModel(RingBuildMaterial& buildMaterial, int modelHandle, int meshHandle) {}

void ResourceManager::UpdateEngineModel(SphereBuildMaterial& buildMaterial, int modelHandle, int meshHandle) {

	std::shared_ptr <SphereMesh> newSphere;
	newSphere = std::make_shared<SphereMesh>();

	if (buildMaterial.LatitudeSegments != 0)	newSphere->SetLat(buildMaterial.LatitudeSegments);
	if (buildMaterial.LongitudeSegments != 0)	newSphere->SetLong(buildMaterial.LongitudeSegments);

	newSphere->CreateVertexResource_(BDevice_);
	newSphere->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newSphere);

	SwapMeshAndModelGroup(newSphere, modelHandle, meshHandle);
}

void ResourceManager::UpdateEngineModel(CylinderBuildMaterial& buildMaterial, int modelHandle, int meshHandle) {

	std::shared_ptr <CylinderMesh> newCylinder;
	newCylinder = std::make_shared<CylinderMesh>();

	if (buildMaterial.Division != 0)	newCylinder->SetCylinderDivide(buildMaterial.Division);
	if (buildMaterial.TopRadius != 0)	newCylinder->SetTopRadius(buildMaterial.TopRadius);
	if (buildMaterial.BottomRadius != 0)	newCylinder->SetBottomRadius(buildMaterial.BottomRadius);
	if (buildMaterial.Height != 0)	newCylinder->SetHeight(buildMaterial.Height);
	newCylinder->SetReverseY(buildMaterial.isReverseY);

	newCylinder->CreateVertexResource_(BDevice_);
	newCylinder->CreateIndexResource_(BDevice_);
	meshBufferList_.push_back(newCylinder);

	SwapMeshAndModelGroup(newCylinder, modelHandle, meshHandle);
}


void ResourceManager::SwapMeshAndModelGroup(std::shared_ptr<Model> model, int modelHandle, int modelIndex) {

	/// ................ 資料収集/チェック ................ ///

	/// ハンドルが正しいか確認
	if (modelHandle < 0 || modelHandle >= (int)modelGroupList_.size()) {
		Logger::Log("[kEngine] RM : SwapMeshAndModelGroup() ModelGroup handle %d not found.", modelHandle);
		return;
	}

	/// まずmodelGroupListの[0]番のMeshのハンドルを取得する。
	int meshHandle = modelGroupList_[modelHandle]->GetMeshHandle(0);

	bool isNewModelGroup = (modelIndex == -1);

	/// modelIndexが正しいか確認
	if(!isNewModelGroup) {
		if (modelIndex < 0 || modelIndex >= (int)modelGroupList_[modelHandle]->GetModelNum()) {
			Logger::Log("[kEngine] RM : SwapMeshAndModelGroup() Model index %d not found.", modelIndex);
			return;
		}

		auto modelGroup = modelGroupList_[modelHandle];
		meshHandle = modelGroup->GetMeshHandle(modelIndex);
	}

	/// このに走ると必ずmeshに何かの問題があり
	if (meshHandle < 0 || meshHandle >= (int)meshBufferList_.size()) {
		Logger::Log("[kEngine] RM : SwapMeshAndModelGroup() Mesh index %d not found.", meshHandle);
		assert(false);
	}

	/// ................... 正式操作 .................... ///

	/// MeshBufferListのMeshを新しいMeshに入れ替える
	meshBufferList_[meshHandle] = model;

	/// 
	if(isNewModelGroup){
		auto modelGroup = std::make_shared<ModelGroup>();
		modelGroup->PushModel(model);
		modelGroup->PushMeshHandle(meshHandle);
		modelGroupList_[modelHandle] = modelGroup;
	} else {
		auto modelGroup = modelGroupList_[modelHandle];
		modelGroup->SwapModel(modelIndex, model, meshHandle);
	}
};

int ResourceManager::LoadModel(std::string Path) {
	/// Resourceに同じものがあるがどうか捜索
	auto target = std::find_if(
		modelGroupList_.begin(),
		modelGroupList_.end(),
		[&](std::shared_ptr <ModelGroup> ptr) {return ptr->GetFilePath() == Path; }
	);
	if (target != modelGroupList_.end()) {
		return (int)std::distance(modelGroupList_.begin(), target);
	}

	/// モデルの読み込み
	int ModelGroupHandle = CreateModelResource(Path);

	/// 各モテルのテキスチャーを設定する
	for (int i = 0; i < modelGroupList_[ModelGroupHandle]->GetModelNum(); i++) {
		SetModelTexture(modelGroupList_[ModelGroupHandle]->GetModel(i));
	}
	return ModelGroupHandle;

}

int ResourceManager::ReadFile(std::string Path) {

	/// あるからそのハンドルを返す
	if (modelDataList_.contains(Path)) {
		for (const auto& pair : modelDataHandleMap_) {
			if (pair.second == Path) {
				return pair.first;
			}
		}
	}

	/// ないからモデルグループを作成
	auto modelData = std::make_shared<ModelData>(LoadModelBranch(Path));

	/// Listに追加
	modelDataList_[Path] = modelData;
	modelDataHandleMap_[modelDataCounter_] = Path;

	/// ハンドルを返し、カウンター＋1
	return modelDataCounter_++;
}

std::shared_ptr<ModelData> ResourceManager::GetModelData(int handle) {

	if (!modelDataHandleMap_.contains(handle)) {
		Logger::Log("kEngine:: RM :: ModelData handle %d not found.", handle);
		return nullptr;
	} else {
		return modelDataList_[modelDataHandleMap_[handle]];
	}

}

int ResourceManager::LoadCommonTexture(const std::string& filePath) {
	/// 同じテクスチャーがあるか確認、あったらそのハンドルを返す
	int textureHandle = TextureManager::GetInstance()->CheckSameCommonTextureLoaded(filePath);
	if (textureHandle != -1) return textureHandle;

	/// テクスチャー読み込み
	textureHandle = TextureManager::GetInstance()->LoadCommonTexture(filePath);

	return textureHandle;
}

int ResourceManager::LoadModelTexture(const std::string& filePath) {
	/// 同じテクスチャーがあるか確認、あったらそのハンドルを返す
	int textureHandle = TextureManager::GetInstance()->CheckSameModelTextureLoaded(filePath);
	if (textureHandle != -1) return textureHandle;

	/// テクスチャー読み込み
	textureHandle = TextureManager::GetInstance()->LoadModelTexture(filePath);

	return textureHandle;
}

int ResourceManager::GetTextureHandleFromCommonList(int index) {
	return TextureManager::GetInstance()->GetCommonTextureHandle(index);
}

int ResourceManager::GetTextureHandleFromModelGroup(int modelHandle, int part) {
	return modelGroupList_[modelHandle]->GetModel(part)->GetTextureHandle();
}

DirectX::TexMetadata ResourceManager::GetTextureMetaData(int textureHandle) {
	return TextureManager::GetInstance()->GetTextureMetadata(textureHandle);
}

int ResourceManager::ReadModelTextureHandle(int index) {
	return TextureManager::GetInstance()->GetModelTextureHandle(index);
}

int ResourceManager::ReadCommonTextureHandle(int index) {
	return TextureManager::GetInstance()->GetCommonTextureHandle(index);
}

bool ResourceManager::SetModelTexture(Model* model) {
	if (!model->GetTexturePatch().empty()) {
		model->SetTextureHandle(TextureManager::GetInstance()->LoadModelTexture(model->GetTexturePatch()));
		return true;
	}
	model->SetTextureHandle(TextureManager::GetDefaultTextureHandle() + 1);
	return false;
}

void ResourceManager::ResizeSimpleSpriteMesh(DirectX::TexMetadata Metadata, int counter, CornerData corner, Vector2 anchorPoint, Vector2 cropLT, Vector2 cropSize) {

	/// 足りない分を作成
	ResizeSimpleSpriteMeshList(counter);

	/// スブライドサイズ
	Vector2 texSize{ (float)Metadata.width, (float)Metadata.height };

	/// サイズ調整
	if (anchorPoint != Vector2{ 0, 0 }) {
		simpleSpriteMeshList_[counter]->SetAnchor(texSize, anchorPoint);
	} else if (!CheckCornerDataDefault(corner)) {
		simpleSpriteMeshList_[counter]->SetSize(corner);
	} else {
		simpleSpriteMeshList_[counter]->SetSize(texSize);
	}

	if (cropLT != Vector2{} || cropSize != Vector2{}) {
		simpleSpriteMeshList_[counter]->SetTexcoord(texSize, cropLT, cropSize);
	} else {
		if (!simpleSpriteMeshList_[counter]->CheckIsDefaultSize()) {
			simpleSpriteMeshList_[counter]->ResetTexcoord();
		}
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetTextureCPUDescriptorHandle(int handle) {
	return TextureManager::GetInstance()->GetTextureCPUDescriptorHandle(handle);
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceManager::GetTextureGPUDescriptorHandle(int handle) {
	return TextureManager::GetInstance()->GetTextureGPUDescriptorHandle(handle);
}

void ResourceManager::ResizeSimpleSpriteMeshList(int spriteNumber) {
	int notEnoughCounter = spriteNumber - (int)simpleSpriteMeshList_.size() + 1;
	/// どれくらい足りないのか計算
	if (notEnoughCounter <= 0)return;
	for (int i = 0; i < notEnoughCounter; i++) {
		CreateSimpleSpriteMeshResource();
	}
}

void ResourceManager::DeleteExtraSpriteMesh(int spriteNumber) {
	int counter = spriteNumber - (int)simpleSpriteMeshList_.size() + 1;
	if (counter >= 0)return;
	for (int i = 0; i < -counter; i++) {
		simpleSpriteMeshList_.pop_back();
	}
}

int ResourceManager::InputMaterialConfig(std::shared_ptr<MaterialConfig> material) {

	/// ================================= 既にあるか確認 =================================== ///
	MaterialEntry* samePtrEntry = nullptr;
	MaterialEntry* sameValueEntry = nullptr;

	for (auto& entry : materialList_) {

		auto locked = entry.config.lock();
		if (!locked) continue;

		// pointerが一緒なら更新してIDを返す
		if (locked.get() == material.get()) {
			samePtrEntry = &entry;
			break;
		}

		// pointerが違っても値が一緒ならIDを返す
		if (*locked == *material) {
			sameValueEntry = &entry;
		}
	}
	/// =========================== 既にある場合、更新してIDを返す =========================== ///
	if (samePtrEntry) {

		// CPUマテリアル更新 
		samePtrEntry->cpuMaterial->inputMaterialConfig(*material);
		
		// GPUマテリアル更新
		MaterialForGPU* gpuPtr = nullptr;
		samePtrEntry->gpuMaterial->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&gpuPtr));
		*gpuPtr = *samePtrEntry->cpuMaterial;
		samePtrEntry->gpuMaterial->GetResource()->Unmap(0, nullptr);

		return samePtrEntry->materialID;
	}

	/// =========================== ない場合、まず完全一致なマテリアルを探す =========================== ///
	if (sameValueEntry) {
		return sameValueEntry->materialID;
	}

	/// ======================== それでもない場合、新しいMaterialEntryを作成 ======================== ///
	MaterialEntry entry;

	/// MaterialIDを設定
	entry.materialID = materialCounter_;
	materialCounter_++;

	/// MaterialConfigのweak_ptrを保存
	entry.config = material;
	entry.textureHandle = material->textureHandle;

	/// 新しいResourceを追加
	auto newResource = std::make_unique<BasicResource>();
	entry.gpuMaterial = newResource.get();
	entry.gpuMaterial->CreateResourceClass_(BDevice_, sizeof(MaterialForGPU));

	/// MaterialとMapする
	entry.cpuMaterial = std::make_unique<MaterialForGPU>();
	entry.cpuMaterial->inputMaterialConfig(*material);

	MaterialForGPU* gpuPtr = nullptr;
	newResource->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&gpuPtr));
	*gpuPtr = *entry.cpuMaterial;
	newResource->GetResource()->Unmap(0, nullptr);

	newResource->SetName("MaterialResource" + std::to_string(entry.materialID));

	materialResourceList_.push_back(std::move(newResource));
	materialList_.push_back(std::move(entry));
	idToIndex_.emplace(materialList_.back().materialID, (int)materialList_.size() - 1);

	return materialList_.back().materialID;
}

/// ================================ RenderTexture制作用 ================================ ///

Microsoft::WRL::ComPtr<ID3D12Resource> ResourceManager::CreateRenderTextureResource(
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	const Vector4& clearColor
) {

	///  textureの元、時関があればこれをセーブして以降使う<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	/// 1. metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(width);
	resourceDesc.Height = UINT(height);
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		   // Textureの次元数。普段使ってるのは2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;		   // RenderTargetとして使うためのフラグ


	/// 2.利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	/// 4.色のクリア設定
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	/// 3.Resourceを生成する
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = BDevice_->CreateCommittedResource(
		&heapProperties,					// Heapの設定
		D3D12_HEAP_FLAG_NONE,				// Heapの特殊な設定。特になし。
		&resourceDesc,						// Resourceの設定
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	// 初回のResourceState。RenderTargetとして使うのでRenderTarget状態
		&clearValue,						// Clear最適値。
		IID_PPV_ARGS(&resource));			// 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	resource->SetName(L"RenderTextureResource");

	return resource;
}

void ResourceManager::CreateRTV(ID3D12Resource* renderTexture, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) {

	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	BDevice_->CreateRenderTargetView(renderTexture, &desc, rtvHandle);
}

void ResourceManager::CreateSRV(ID3D12Resource* renderTexture, DXGI_FORMAT format, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle) {

	/// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC renderTargetSRVDesc{};
	renderTargetSRVDesc.Format = format;
	renderTargetSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	renderTargetSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	renderTargetSRVDesc.Texture2D.MipLevels = 1;

	/// SRVを作成
	BDevice_->CreateShaderResourceView(renderTexture, &renderTargetSRVDesc, srvHandle);
}

RenderTexture ResourceManager::CreateRenderTexture(
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	const Vector4& clearColor
) {

	/// RTVの設定
	auto renderTextureResource = CreateRenderTextureResource(
		width,
		height,
		format,
		clearColor
	);

	/// Alloとり
	uint32_t rtvIndex = RtvManager::GetInstance()->Allocate();

	/// RTV
	CreateRTV(renderTextureResource.Get(), format, RtvManager::GetInstance()->GetCPUDescriptorHandle(rtvIndex));

	/// Alloとり
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();

	/// SRVの作成
	CreateSRV(renderTextureResource.Get(), format, SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex));


	RenderTexture renderTexture{};
	renderTexture.resource = renderTextureResource;
	renderTexture.rtvHandleCPU = RtvManager::GetInstance()->GetCPUDescriptorHandle(rtvIndex);
	renderTexture.srvHandleCPU = SrvManager::GetInstance()->GetCPUDescriptorHandle(srvIndex);
	renderTexture.srvHandleGPU = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);
	renderTexture.width = width;
	renderTexture.height = height;
	renderTexture.format = format;
	renderTexture.clearColor = clearColor;
	renderTexture.currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	CreateDepthStencilForRenderTexture(renderTexture);

	return renderTexture;
}

/// ================================= DepthStencil制作用 ================================ ///
ID3D12Resource* ResourceManager::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {

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



	resource->SetName(L"DepthStencilResource");

	char buffer[128];
	sprintf_s(buffer, "Create resource at %p\n", resource);
	OutputDebugStringA(buffer);

	return resource;
}


void ResourceManager::CreateDepthStencilForRenderTexture(RenderTexture& rt) {
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = rt.width;
	desc.Height = rt.height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// VRAM上に作る

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;


	HRESULT hr = BDevice_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&rt.depthResource)
	);
	assert(SUCCEEDED(hr));

	// 建立 DSV
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	uint32_t dsvIndex = DsvManager::GetInstance()->Allocate();
	rt.dsvHandleCPU = DsvManager::GetInstance()->GetCPUDescriptorHandle(dsvIndex); // 你自己的 DSV heap allocator
	BDevice_->CreateDepthStencilView(rt.depthResource.Get(), &dsvDesc, rt.dsvHandleCPU);
}

void ResourceManager::CleanupUnusedMaterials() {
	std::vector<MaterialEntry> newMaterialList;
	std::vector<std::unique_ptr<BasicResource>> newResourceList;

	newMaterialList.reserve(materialList_.size());
	newResourceList.reserve(materialResourceList_.size());

	for (size_t i = 0; i < materialList_.size(); ++i) {
		if (!materialList_[i].config.expired()) {
			newMaterialList.push_back(std::move(materialList_[i]));
			newResourceList.push_back(std::move(materialResourceList_[i]));
		}
	}

	materialList_ = std::move(newMaterialList);
	materialResourceList_ = std::move(newResourceList);

	idToIndex_.clear();
	for (int i = 0; i < static_cast<int>(materialList_.size()); ++i) {
		idToIndex_[materialList_[i].materialID] = i;
	}
}