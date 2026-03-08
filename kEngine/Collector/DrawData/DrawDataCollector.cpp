#include "Logger.h"
#include "DrawDataCollector.h"
#include "CameraManager/CameraManager.h"
#include "LightManager/LightManager.h"
#include "ResourceManager.h"


void DrawDataCollector::Initialize(CameraManager * cm, LightManager * lm) {
	cameraManager_ = cm, lightManager_ = lm;
}

void DrawDataCollector::Finalize() {
	/// bucketクリア
	opaqueBucket2D_.clear();
	transparentBucket2D_.clear();
	opaqueBuckets3D_.clear();
	transparentBucket3D_.clear();
}

void DrawDataCollector::PreCollect() {

	/// bucketクリア
	opaqueBucket2D_.clear();
	transparentBucket2D_.clear();
	opaqueBuckets3D_.clear();
	transparentBucket3D_.clear();

	/// レイヤードスプライト用リストクリア
	simpleSpriteCounter_ = 0;
	unlayeredSpriteCounter_ = 0;

	/// インスタンスリストクリア
	instanceCounter2D_ = 0;
	instanceCounter3D_ = 0;
}

void DrawDataCollector::EndCollect() {

	/// ネールスキップ
	if (opaqueBucket2D_.empty() && transparentBucket2D_.empty()&&
		opaqueBuckets3D_.empty() && transparentBucket3D_.empty())
	return;

	/// 実際のインスタンスリスト作成
	BuildInstanceList2D();
	BuildInstanceList3D();
	
}

#pragma region /// ===================================== 2D関連 ===================================== ///

void DrawDataCollector::Collect2D(SpriteData* sprite) {
	/// nullチェック
	if (!sprite)return;

	for (auto& object : sprite->objectParts_) {

		/// ========================================  RenderData作成  ========================================///

		/// metaData
		DirectX::TexMetadata metaData = ResourceManager::GetInstance()->GetTextureMetaData(object.materialConfig->textureHandle);

		/// スプライトメッシュのリサイズ
		ResourceManager::GetInstance()->ResizeSimpleSpriteMesh(
			metaData,
			simpleSpriteCounter_,
			object.conerData,
			object.anchorPoint,
			object.cropLT,
			object.cropSize
		);

		/// RenderData作成
		RenderData renderData;

		/// メッシュ設定
		renderData.mesh = ResourceManager::GetInstance()->simpleSpriteMeshList_[simpleSpriteCounter_];

		/// マテリアル設定
		object.materialConfig->MakeUVMatrix();
		renderData.materialID = ResourceManager::GetInstance()->InputMaterialConfig(object.materialConfig);

		/// 変換行列設定
		renderData.transformData = SpriteWVPAdjustment2D(*sprite, object);

		/// PSO設定
		renderData.psoID = PSODecision(*object.materialConfig);

		/// サブメッシュインデックス設定
		renderData.subMeshIndex = 0;

		/// ========================================  バケット振り分け  ========================================///
		AddSpriteToBucket2D(renderData, simpleSpriteCounter_);

		/// スプライトカウンターインクリメント
		simpleSpriteCounter_++;
	}
}

float DrawDataCollector::SpriteLayerManagement(float zBuffer) {

	if (zBuffer == 0) {
		if (unlayeredSpriteCounter_ > 4000) {
			Logger::Log("[kWarning]DDC:unlayered Sprite meet max number!");
			return (float)unlayeredSpriteCounter_ * -layerDepth_Sprite;
		}
		float z = (float)unlayeredSpriteCounter_ * -layerDepth_Sprite;
		unlayeredSpriteCounter_++;
		return z;
	} else {
		float z = -layeredSpriteDepth_ + zBuffer * -layerDepth_Sprite;
		if (z < -1.0f) {
			Logger::Log("[kWarning]DDC:layered Sprite meet max number!");
			return -1.0f;
		}
		return z;
	}
	Logger::Log("[kWarning]DDC:Unknow Error!");
	return 0.0f;
}

Matrix4x4 DrawDataCollector::MakeFollowObjectMatrix2D(SpriteData* sprite) {

	float zBuffer = sprite->mainPosition.transform.translate.z;

	Matrix4x4 objectMainMatrix = MakeAffineMatrix(
		sprite->mainPosition.transform.scale,
		sprite->mainPosition.transform.rotate,
		sprite->mainPosition.transform.translate
	);

	Matrix4x4 parentMatrix = Identity();

	SpriteData* parent = sprite->followObject_;
	while (parent != nullptr) {
		Matrix4x4 local = MakeAffineMatrix(
			parent->mainPosition.transform.scale,
			parent->mainPosition.transform.rotate,
			parent->mainPosition.transform.translate
		);

		parentMatrix = local * parentMatrix;
		parent = parent->followObject_;
	}

	Matrix4x4 resultMatrix = objectMainMatrix * parentMatrix;
	resultMatrix.m[3][2] = zBuffer;

	return resultMatrix;
}

TransformationMatrix DrawDataCollector::SpriteWVPAdjustment2D(SpriteData& sprite, SpritePart& part) {

	// 2D UI 用：View 取單位矩陣，Projection 用螢幕尺寸的正交矩陣（左上原點，Y 向下）
	Matrix4x4 viewMatrix = Identity();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
		0.0f,                                     // left
		0.0f,                                     // top
		static_cast<float>(config::GetClientWidth()),   // right
		static_cast<float>(config::GetClientHeight()),  // bottom
		-1.0f,                                    // nearZ，留空間給負的 layer depth
		1.0f                                      // farZ
	);

	Matrix4x4 followWorldMatrix = MakeFollowObjectMatrix2D(&sprite);

	Matrix4x4 localMatrix = MakeAffineMatrix(
		part.transform.scale,
		part.transform.rotate,
		part.transform.translate
	);

	Matrix4x4 worldMatrix = localMatrix * followWorldMatrix;

	float spriteTransform = part.transform.translate.z;
	worldMatrix.m[3][2] = SpriteLayerManagement(spriteTransform);

	TransformationMatrix result{};
	result.world = worldMatrix;
	result.WVP = worldMatrix * viewMatrix * projectionMatrix;
	result.WorldInverseTranspose = worldMatrix.Inverse().Transpose();
	return result;
}

void DrawDataCollector::AddSpriteToBucket2D(RenderData& renderData, int meshID) {

	auto checker = ResourceManager::GetInstance()->idToIndex_.find(renderData.materialID);

	if (checker == ResourceManager::GetInstance()->idToIndex_.end()) {
		Logger::Log("[kError]DDC:MaterialID not found in ResourceManager!");
		return;
	} else {
		Material* material = ResourceManager::GetInstance()->materialList_[checker->second].cpuMaterial.get();

		if (material->color.w < 1.0f) {

			float z = renderData.transformData.WVP.m[3][2];

			auto checker2 = std::find_if(
				transparentBucket2D_.begin(),
				transparentBucket2D_.end(),
				[z](const RenderData& data) {
					float dataZ = data.transformData.WVP.m[3][2];
					return z > dataZ;
				}
			);

			if (checker2 == transparentBucket2D_.end()) {
				transparentBucket2D_.emplace_back(renderData);
			} else {
				std::size_t index = std::distance(transparentBucket2D_.begin(), checker2);
				transparentBucket2D_.insert(transparentBucket2D_.begin() + index, renderData);
			}
		} else {
			/// 不透明オブジェクトバケットへ追加
			opaqueBucket2D_[static_cast<PSOType>(renderData.psoID)][renderData.materialID][meshID].emplace_back(renderData);
		}
	}
}

#pragma endregion

#pragma region /// ===================================== 3D関連 ===================================== ///

void DrawDataCollector::Collect3D(ObjectData* object) {
	/// nullチェック
	if (!object)return;

	int moderCounter = 0;
	for (auto& objectPart : object->objectParts_) {

		/// ========================================  RenderData作成  ========================================///

		/// RenderData作成
		RenderData renderData;

		/// メッシュ設定
		Model* modelData = ResourceManager::GetInstance()->modelGroupList_[object->modelHandle_]->GetModel(moderCounter);
		renderData.mesh = modelData;

		/// マテリアル設定
		objectPart.materialConfig->MakeUVMatrix();
		renderData.materialID = ResourceManager::GetInstance()->InputMaterialConfig(objectPart.materialConfig);

		/// 変換行列設定
		renderData.transformData = ObjectWVPAdjustment3D(*object, objectPart, modelData->GetModelData());

		/// PSO設定
		renderData.psoID = PSODecision(*objectPart.materialConfig);

		/// サブメッシュインデックス設定
		renderData.subMeshIndex = 0;

		/// ========================================  バケット振り分け  ========================================///
		AddObjectToBucket3D(renderData, object->modelHandle_);

		moderCounter++;
	}
}

Matrix4x4 DrawDataCollector::MakeFollowObjectMatrix3D(ObjectData* object) {

	Matrix4x4 objectMainMatrix = MakeAffineMatrix(
		object->mainPosition.transform.scale,
		object->mainPosition.transform.rotate,
		object->mainPosition.transform.translate
	);

	Matrix4x4 parentMatrix = Identity();

	ObjectPart* parent = object->followObject_;
	while (parent != nullptr) {
		Matrix4x4 local = MakeAffineMatrix(
			parent->transform.scale,
			parent->transform.rotate,
			parent->transform.translate
		);

		parentMatrix = local * parentMatrix;
		parent = parent->parentPart;
	}

	return objectMainMatrix * parentMatrix;
}

TransformationMatrix DrawDataCollector::ObjectWVPAdjustment3D(ObjectData& object, ObjectPart& part, ModelData modelData) {
	Camera* cam = cameraManager_->GetActiveCamera();
	Matrix4x4 viewMatrix = cam->GetViewMatrix();
	Matrix4x4 projectionMatrix = cam->GetProjectionMatrix();

	Matrix4x4 followWorldMatrix = MakeFollowObjectMatrix3D(&object);

	// Billboard 子物件：XY 旋轉を0にする
	if (object.isBillboard_) {
		Vector3 camRot = cam->GetTransform().rotate;
		part.transform.rotate.x = camRot.x;
		part.transform.rotate.y = camRot.y;
		part.transform.rotate.z = 0.0f;
	}

	Matrix4x4 partWorldMatrix = part.UpdateWorldMatrix();
	Matrix4x4 worldMatrix = partWorldMatrix * followWorldMatrix;

	TransformationMatrix result{};
	result.WVP = modelData.rootNode.localMatrix * worldMatrix * viewMatrix * projectionMatrix;
	result.world = modelData.rootNode.localMatrix * worldMatrix;
	result.WorldInverseTranspose = worldMatrix.Inverse().Transpose();
	return result;
}

void DrawDataCollector::AddObjectToBucket3D(RenderData& renderData,int meshID) {

	auto checker = ResourceManager::GetInstance()->idToIndex_.find(renderData.materialID);

	if (checker == ResourceManager::GetInstance()->idToIndex_.end()) {
		Logger::Log("[kError]DDC:MaterialID not found in ResourceManager!");
		return;
	} else {
		Material* material = ResourceManager::GetInstance()->materialList_[checker->second].cpuMaterial.get();

		if (material->color.w < 1.0f) {

			float z = renderData.transformData.world.m[3][2];

			auto checker2 = std::find_if(
				transparentBucket3D_.begin(),
				transparentBucket3D_.end(),
				[z](const RenderData& data) {
					float dataZ = data.transformData.world.m[3][2];
					return z > dataZ;
				}
			);

			if (checker2 == transparentBucket3D_.end()) {
				transparentBucket3D_.emplace_back(renderData);
			} else {
				std::size_t index = std::distance(transparentBucket3D_.begin(), checker2);
				transparentBucket3D_.insert(transparentBucket3D_.begin() + index, renderData);
			}
		} else {
			/// 不透明オブジェクトバケットへ追加
			opaqueBuckets3D_[static_cast<PSOType>(renderData.psoID)][renderData.materialID][meshID].emplace_back(renderData);
		}
	}

	auto& t = renderData.transformData;
}

#pragma endregion

#pragma region /// ================================= パーティクル関連 ================================== ///

void DrawDataCollector::CollectParticleC(ObjectData* object) {
	/// nullチェック
	if (!object)return;

	int moderCounter = 0;
	for (auto& objectPart : object->objectParts_) {

		/// ========================================  RenderData作成  ========================================///

		/// RenderData作成
		RenderData renderData;

		/// メッシュ設定
		Model* modelData = ResourceManager::GetInstance()->modelGroupList_[object->modelHandle_]->GetModel(moderCounter);
		renderData.mesh = modelData;

		/// マテリアル設定
		objectPart.materialConfig->MakeUVMatrix();
		renderData.materialID = ResourceManager::GetInstance()->InputMaterialConfig(objectPart.materialConfig);

		/// 変換行列設定
		renderData.transformData = ObjectWVPAdjustmentPC(*object, objectPart, modelData->GetModelData());

		/// PSO設定
		renderData.psoID = PSODecision(*objectPart.materialConfig);

		/// サブメッシュインデックス設定
		renderData.subMeshIndex = 0;

		/// ========================================  バケット振り分け  ========================================///
		AddObjectToBucketPC(renderData);

		moderCounter++;
	}
}

TransformationMatrix DrawDataCollector::ObjectWVPAdjustmentPC(ObjectData& object, ObjectPart& part, ModelData modelData) {
	Camera* cam = cameraManager_->GetActiveCamera();
	Matrix4x4 viewMatrix = cam->GetViewMatrix();
	Matrix4x4 projectionMatrix = cam->GetProjectionMatrix();

	Matrix4x4 followWorldMatrix = MakeFollowObjectMatrix3D(&object);

	// Billboard 子物件：XY 旋轉を0にする
	if (object.isBillboard_) {
		Vector3 camRot = cam->GetTransform().rotate;
		part.transform.rotate.x = camRot.x;
		part.transform.rotate.y = camRot.y;
		part.transform.rotate.z = 0.0f;
	}

	Matrix4x4 partWorldMatrix = part.UpdateWorldMatrix();
	Matrix4x4 worldMatrix = partWorldMatrix * followWorldMatrix;

	TransformationMatrix result{};
	result.WVP = modelData.rootNode.localMatrix * worldMatrix * viewMatrix * projectionMatrix;
	result.world = modelData.rootNode.localMatrix * worldMatrix;
	result.WorldInverseTranspose = worldMatrix.Inverse().Transpose();
	return result;
}

void DrawDataCollector::AddObjectToBucketPC(RenderData& renderData) {

	auto checker = ResourceManager::GetInstance()->idToIndex_.find(renderData.materialID);

	if (checker == ResourceManager::GetInstance()->idToIndex_.end()) {
		Logger::Log("[kError]DDC:MaterialID not found in ResourceManager!");
		return;
	} else {
		bucketParticleC_.emplace_back(renderData);
	}

	auto& t = renderData.transformData;
}

#pragma endregion

#pragma region /// ========================== カメラ/マテリアル/ライティング関連 ========================= ///

void DrawDataCollector::BuildInstanceList2D() {

	if (opaqueBucket2D_.empty() && transparentBucket2D_.empty())return;

	/// 不透明物件
	for (auto& [psoID, materialBuckets] : opaqueBucket2D_) {
		for (auto& [materialID, RenderDataGroup] : materialBuckets) {
			if (RenderDataGroup.empty()) continue;
			for (auto& [meshBuffer, RenderData] : RenderDataGroup) {
				/// WVP計算
				for (auto& object : RenderData) {
					instancingList2D_[instanceCounter2D_].WVP = object.transformData.WVP;;
					instancingList2D_[instanceCounter2D_].world = object.transformData.world;
					instancingList2D_[instanceCounter2D_].WorldInverseTranspose = object.transformData.WorldInverseTranspose;
					instanceCounter2D_++;
				}
			}
		}
	}

	/// 透明物件
	for (auto& object : transparentBucket2D_) {
		instancingList2D_[instanceCounter2D_].WVP = object.transformData.WVP;;
		instancingList2D_[instanceCounter2D_].world = object.transformData.world;
		instancingList2D_[instanceCounter2D_].WorldInverseTranspose = object.transformData.WorldInverseTranspose;
		instanceCounter2D_++;
	}
}

void DrawDataCollector::BuildInstanceList3D() {
	if (opaqueBuckets3D_.empty() && transparentBucket3D_.empty())return;

	/// 不透明物件
	for (auto& [psoID, materialBuckets] : opaqueBuckets3D_) {
		for (auto& [materialID, RenderDataGroup] : materialBuckets) {
			if (RenderDataGroup.empty()) continue;
			for (auto& [meshBuffer, RenderData] : RenderDataGroup) {
				/// WVP計算
				for (auto& object : RenderData) {
					instancingList3D_[instanceCounter3D_].WVP = object.transformData.WVP;;
					instancingList3D_[instanceCounter3D_].world = object.transformData.world;
					instancingList3D_[instanceCounter3D_].WorldInverseTranspose = object.transformData.WorldInverseTranspose;
					instanceCounter3D_++;
				}
			}
		}
	}

	/// 透明物件
	for (auto& object : transparentBucket3D_) {
		instancingList3D_[instanceCounter3D_].WVP = object.transformData.WVP;;
		instancingList3D_[instanceCounter3D_].world = object.transformData.world;
		instancingList3D_[instanceCounter3D_].WorldInverseTranspose = object.transformData.WorldInverseTranspose;
		instanceCounter3D_++;
	}
}

#pragma endregion

#pragma region /// ========================== カメラ/マテリアル/ライティング関連 ========================= ///

Vector3 DrawDataCollector::GetCameraPosition() const {
	return cameraManager_->GetActiveCamera()->GetTransform().translate;
}

uint32_t DrawDataCollector::PSODecision(MaterialConfig& material) {

	LightModelType lightModelType = (LightModelType)(int)material.lightModelType;

	switch (lightModelType) {
	case LightModelType::Sprite2D:
		return (uint32_t)PSOType::Sprite2D;
	case LightModelType::Lambert:
		return (uint32_t)PSOType::Lambert;
	case LightModelType::HalfLambert:
		return (uint32_t)PSOType::HalfLambert;
	case LightModelType::PhongReflection:
		return (uint32_t)PSOType::PhongReflection;
	case LightModelType::BlinnPhongReflection:
		return (uint32_t)PSOType::BlinnPhongReflection;
	case LightModelType::FlameNeonGlow:
		return (uint32_t)PSOType::FlameNeonGlow;
	}
	return (uint32_t)PSOType::NONE;
}


void DrawDataCollector::UpdateLightData() {
	if(lightManager_)lightManager_->TurnDataToGPUData();
}

std::vector<LightGPU> DrawDataCollector::GetLightGPUBuffer() {
	return lightManager_->GetGPUBuffer();
}

uint32_t DrawDataCollector::GetLightCount() {
	return (uint32_t)lightManager_->GetLightCount();
}

#pragma endregion