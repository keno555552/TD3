#include "ResourceManager.h"
#include <Object/Object.h>

std::unique_ptr <ResourceManager> ResourceManager::instance_ = nullptr;

ResourceManager* ResourceManager::GetInstance() {
	if (instance_ == nullptr) {
		instance_.reset(new ResourceManager);
	}
	return instance_.get();
}

void ResourceManager::Initialize(DirectXCore* device) {

	core_ = device;
	BDevice_ = core_->GetDevice();

	config::default_Sprite2D_MeshBufferHandle_ = CreateSimpleSpriteMeshResource();
	config::default_Triangle_MeshBufferHandle_ = CreateTriangleResource();
	config::default_Cube_MeshBufferHandle_ = CreateCubeResource();
	config::default_Sphere_MeshBufferHandle_ = CreateSphereResource(10);
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
}

void ResourceManager::Destroy() {
	instance_.reset();
}

void ResourceManager::CreateTurnResource() {
}


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
	modelGroup->PushModelHandle((int)meshBufferList_.size() - 1);
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
	modelGroup->PushModelHandle((int)meshBufferList_.size() - 1);
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
	modelGroup->PushModelHandle((int)meshBufferList_.size() - 1);
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;

}

int ResourceManager::CreateModelResource(std::string Path) {

	auto modelGroup = std::make_shared<ModelGroup>();
	modelGroup->SetModelObj(Path);

	/// パースによって同じモデルがリストにいるかを確認
	if (!modelGroupList_.empty()) {
		for (int i = 0; i < (int)modelGroupList_.size(); i++) {
			std::string checkPath = modelGroupList_[i]->GetDirectoryPath();
			std::string checkName = modelGroupList_[i]->GetObjName_();
			if (checkPath == modelGroup->GetDirectoryPath()) {
				if (checkName == modelGroup->GetObjName_()) {
					return i;
				}
			}
		}
	}

	/// Model読み込み
	std::vector<ModelData> modelList = LoadAssimpFile(modelGroup->GetDirectoryPath(), modelGroup->GetObjName_());
	//std::vector<ModelData> modelList = LoadMuitObjFile(modelGroup->GetDirectoryPath(), modelGroup->GetObjName_());
	for (auto& ptr : modelList) {
		auto newModel = std::make_shared<Model>();
		newModel->SetModelData(ptr);
		newModel->SetModelObj(Path);
		newModel->CreateVertexResourceG_(BDevice_);
		modelGroup->PushModel(newModel);
		meshBufferList_.push_back(newModel);
		modelGroup->PushModelHandle((int)meshBufferList_.size() - 1);
	}
	modelGroupList_.push_back(modelGroup);

	return (int)modelGroupList_.size() - 1;
}

int ResourceManager::LoadModel(std::string Path) {
	/// Resourceに同じものがあるがどうか捜索
	auto target = std::find_if(
		modelGroupList_.begin(),
		modelGroupList_.end(),
		[&](std::shared_ptr <ModelGroup> ptr) {return ptr->GetFullPath_() == Path; }
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

int ResourceManager::GetTextureCounter() {
	return TextureManager::GetInstance()->GetTextureCounter();
}

void ResourceManager::TextureCounterPlus(int index) {
	TextureManager::GetInstance()->TextureCounterPlus(index);
}

void ResourceManager::TextureCounterAdjust(int index) {
	TextureManager::GetInstance()->TextuerCounterAdjust(index);
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
	auto checker = std::find_if(
		materialList_.begin(),
		materialList_.end(),
		[&](MaterialEntry& entry) {
			auto locked = entry.config.lock();
			return locked && (locked.get() == material.get());


			//auto locked = entry.config.lock().get();
			//Logger::Log("Comparing material config pointers: %p and %p", locked, material.get());
			//return locked == material.get();
		}
	);

	/// =========================== 既にある場合、更新してIDを返す =========================== ///
	if (checker != materialList_.end()) {

		/// pointerが一緒だから更新なくてok
		//checker->config = material;

		/// 更新 CPU 材質
		checker->cpuMaterial->inputMaterialConfig(*material.get());

		/// 更新 GPU 材質
		MaterialForGPU* gpuPtr = nullptr;
		checker->gpuMaterial->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&gpuPtr));
		*gpuPtr = *checker->cpuMaterial;
		checker->gpuMaterial->GetResource()->Unmap(0, nullptr);

		return checker->materialID;
	}

	/// ======================== ない場合、新しいMaterialEntryを作成 ======================== ///
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

	materialResourceList_.push_back(std::move(newResource));
	materialList_.push_back(std::move(entry));
	idToIndex_.emplace(materialList_.back().materialID, (int)materialList_.size() - 1);

	return materialList_.back().materialID;
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
