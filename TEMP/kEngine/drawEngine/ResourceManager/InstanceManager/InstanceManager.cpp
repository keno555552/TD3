#include "InstanceManager.h"
#include "Object/Object.h"

void InstanceManager::Finalize() {
	ClearInstance(materialConfigList_);
	ClearInstance(spriteList_);
	ClearInstance(tile2DList_);
	ClearInstance(modelList_);
	ClearInstance(tile3DList_);
}

void InstanceManager::Update() {

	UpdateInstance(materialConfigList_);
	UpdateInstance(spriteList_);
	UpdateInstance(tile2DList_);
	UpdateInstance(modelList_);
	UpdateInstance(tile3DList_);


	if (tileLayerCount != 0) {
		tileLayerCount = 0;
	}
	if (spriteLayerCount > 1) {
		spriteLayerCount = (int)(spriteList_[spriteList_.size() - 1]->position.z + 0.0001f);
	}
}

//void InstanceManager::AddSpriteInstance(Vector2 pos, MaterialConfig material) {
//	SpriteInstance instance;
//	instance.position = { pos.x,pos.y,(float)tileLayerCount * -0.0001f };
//	instance.scale = { 1.0f,1.0f };		/// まだ使ってない
//	instance.rotate = { 0.0f,0.0f,0.0f };  /// まだ使ってない
//	instance.layer = 0;					/// まだ使ってない
//	instance.drawState = STANDBY;
//
//	auto checker = std::find_if(materialConfigList_.begin(),
//		materialConfigList_.end(),
//		[&](MaterialConfig* ptr) {return *ptr == material; });
//
//	if (checker == materialConfigList_.end()) {
//		MaterialConfig* newMaterial = new MaterialConfig(material);
//		materialConfigList_.push_back(newMaterial);
//		instance.materialConfigIndex = int(materialConfigList_.size() - 1);
//	} else {
//		instance.materialConfigIndex = (int)std::distance(materialConfigList_.begin(), checker);
//	}
//
//	auto checker2 = std::find_if(spriteList_.begin(),
//		spriteList_.end(),
//		[&](SpriteInstance* ptr) {return ptr->CheckSame(instance); });
//
//	if (checker2 != spriteList_.end()) {
//		(*checker2)->drawState = STANDBY;
//		(*checker2)->materialConfigIndex = instance.materialConfigIndex;
//	} else {
//		SpriteInstance* newInstance = new SpriteInstance(instance);
//		spriteList_.push_back(newInstance);
//	}
//	spriteLayerCount++;
//}

void InstanceManager::AddModelInstance(TransformationMatrix* wvpData, MaterialConfig material, int vertexNum, int modelHandle, bool useDefaultModel) {
	//	ModelInstance instance;
	//	instance.transformData = wvpData;	///参照渡し
	//	instance.modelHandle = modelHandle;
	//	instance.useDefaultModel = useDefaultModel;
	//	instance.vertexNum = vertexNum;
	//	instance.drawState = STANDBY;
	//
	//	auto checker = std::find_if(materialConfigList_.begin(),
	//		materialConfigList_.end(),
	//		[&](MaterialConfig* ptr) {return *ptr == material; });
	//
	//	if (checker == materialConfigList_.end()) {
	//		MaterialConfig* newMaterial = new MaterialConfig(material);
	//		materialConfigList_.push_back(newMaterial);
	//		instance.materialConfigIndex = int(materialConfigList_.size() - 1);
	//	} else {
	//		instance.materialConfigIndex = (int)std::distance(materialConfigList_.begin(), checker);
	//	}
	//
	//	auto checker2 = std::find_if(modelList_.begin(),
	//		modelList_.end(),
	//		[&](ModelInstance* ptr) {return ptr->CheckSame(instance); });
	//
	//	if (checker2 != modelList_.end()) {
	//		(*checker2)->drawState = STANDBY;
	//		(*checker2)->materialConfigIndex = instance.materialConfigIndex;
	//	} else {
	//		ModelInstance* newInstance = new ModelInstance(instance);
	//		modelList_.push_back(newInstance);
	//	}
}

void InstanceManager::Add2DTileInstance(Vector2 pos, MaterialConfig material) {
	SpriteInstance instance;
	instance.position = { pos.x,pos.y,(float)tileLayerCount * -0.0001f };
	instance.scale = { 1.0f,1.0f };		/// まだ使ってない
	instance.rotate = { 0.0f,0.0f,0.0f };  /// まだ使ってない
	instance.layer = 0;					/// まだ使ってない
	instance.drawState = STANDBY;

	auto checker = std::find_if(materialConfigList_.begin(),
		materialConfigList_.end(),
		[&](MaterialConfig* ptr) {return *ptr == material; });

	if (checker == materialConfigList_.end()) {
		MaterialConfig* newMaterial = new MaterialConfig(material);
		materialConfigList_.push_back(newMaterial);
		instance.materialConfigIndex = int(materialConfigList_.size() - 1);
	} else {
		instance.materialConfigIndex = (int)std::distance(materialConfigList_.begin(), checker);
	}

	auto checker2 = std::find_if(tile2DList_.begin(),
		tile2DList_.end(),
		[&](SpriteInstance* ptr) {return ptr->CheckSame(instance); });

	if (checker2 != tile2DList_.end()) {
		(*checker2)->drawState = STANDBY;
		(*checker2)->materialConfigIndex = instance.materialConfigIndex;
	} else {
		SpriteInstance* newInstance = new SpriteInstance(instance);
		tile2DList_.push_back(newInstance);
	}
	tileLayerCount++;
}

void InstanceManager::Add3DTileInstance(TransformationMatrix* wvpData, MaterialConfig material, int vertexNum, int modelHandle, bool useDefaultModel) {
	//
	//	ModelInstance instance;
	//	instance.transformData = wvpData;	///参照渡し
	//	instance.modelHandle = modelHandle;
	//	instance.useDefaultModel = useDefaultModel;
	//	instance.vertexNum = vertexNum;
	//	instance.drawState = STANDBY;
	//
	//	auto checker = std::find_if(materialConfigList_.begin(),
	//		materialConfigList_.end(),
	//		[&](MaterialConfig* ptr) {return *ptr == material; });
	//
	//	if (checker == materialConfigList_.end()) {
	//		MaterialConfig* newMaterial = new MaterialConfig(material);
	//		materialConfigList_.push_back(newMaterial);
	//		instance.materialConfigIndex = int(materialConfigList_.size() - 1);
	//	} else {
	//		instance.materialConfigIndex = (int)std::distance(materialConfigList_.begin(), checker);
	//		materialConfigList_[instance.materialConfigIndex]->drawState = STANDBY;
	//	}
	// 
	// 
	//
	//	auto checker2 = std::find_if(tile3DList_.begin(),
	//		tile3DList_.end(),
	//		[&](ModelInstance* ptr) {return ptr->CheckSame(instance); });
	//
	//	if (checker2 == tile3DList_.end()) {
	//		ModelInstance* newInstance = new ModelInstance(instance);
	//		tile3DList_.push_back(newInstance);
	//	} else {
	//		(*checker2)->drawState = STANDBY;
	//		(*checker2)->materialConfigIndex = instance.materialConfigIndex;
	//	}
}



void InstanceManager::Add2DInstance(Transform wvpData, MaterialConfig material, CornerData cornerData, Vector2 anchorPoint, Vector2 cropLT, Vector2 cropSize) {
	/// 2Dインスタンスを作る
	SpriteInstance instance;
	instance.position = { wvpData.translate.x, wvpData.translate.y };
	instance.scale = { wvpData.scale.x,wvpData.scale.y };
	instance.rotate = wvpData.rotate;
	instance.drawState = STANDBY;

	instance.cornerData = cornerData;
	instance.anchorPoint = anchorPoint;

	instance.cropLT = cropLT;
	instance.cropSize = cropSize;

	/// レイヤー設定
	if (wvpData.translate.z == 0) {
		/// レイヤー指定がない場合は自動設定
		instance.position.z = tileLayerCount * 0.0001f + 0.0002f;
		tileLayerCount++;
	} else {
		/// レイヤー指定がある場合はその値を使用し、深度調整のみ行う
		instance.position.z = layeredSpriteDepth_ + wvpData.translate.z;
	}

	/// マテリアル設定
	auto checker = std::find_if(materialConfigList_.begin(),
		materialConfigList_.end(),
		[&](MaterialConfig* ptr) {return *ptr == material; });

	if (checker == materialConfigList_.end()) {
		MaterialConfig* newMaterial = new MaterialConfig(material);
		materialConfigList_.push_back(newMaterial);
		instance.materialConfigIndex = int(materialConfigList_.size() - 1);
	} else {
		instance.materialConfigIndex = (int)std::distance(materialConfigList_.begin(), checker);
	}

	/// インスタンス重複チェック
	auto checker2 = std::find_if(tile2DList_.begin(),
		tile2DList_.end(),
		[&](SpriteInstance* ptr) {return ptr->CheckSame(instance); });

	if (checker2 != tile2DList_.end()) {
		(*checker2)->drawState = STANDBY;
		(*checker2)->materialConfigIndex = instance.materialConfigIndex;
	} else {
		SpriteInstance* newInstance = new SpriteInstance(instance);
		tile2DList_.push_back(newInstance);
	}

	/// レイヤー管理
	SpriteLayerManagement();
}

void InstanceManager::Add3DInstance(TransformationMatrix wvpData, MaterialConfig material, int vertexNum, int modelHandle, int modelGroupHandle) {
	ModelInstance instance{};
	instance.transformData = wvpData;	///参照渡し
	instance.modelHandle = modelHandle;
	instance.modelGroupHandle = modelGroupHandle;
	instance.vertexNum = vertexNum;
	instance.drawState = STANDBY;

	auto checker = std::find_if(materialConfigList_.begin(),
		materialConfigList_.end(),
		[&](MaterialConfig* ptr) {return *ptr == material; });

	if (checker == materialConfigList_.end()) {
		MaterialConfig* newMaterial = new MaterialConfig(material);
		materialConfigList_.push_back(newMaterial);
		instance.materialConfigIndex = int(materialConfigList_.size() - 1);
	} else {
		instance.materialConfigIndex = (int)std::distance(materialConfigList_.begin(), checker);
		materialConfigList_[instance.materialConfigIndex]->drawState = STANDBY;
	}

	auto checker2 = std::find_if(tile3DList_.begin(),
		tile3DList_.end(),
		[&](ModelInstance* ptr) {return ptr->CheckSame(instance); });

	if (checker2 == tile3DList_.end()) { 
		ModelInstance* newInstance = new ModelInstance(instance);
		tile3DList_.push_back(newInstance);
	} else {
		(*checker2)->drawState = STANDBY;
		(*checker2)->materialConfigIndex = instance.materialConfigIndex;
	}
}

void InstanceManager::Add3DInstance(ObjectData* object) {
	//for(auto& objectPart : object->objectParts_){
	//	if (objectPart.materialConfig->textureColor.w < 1.0f) {
	//		transparentObjectParts_.push_back(&objectPart);
	//	} else {
	//		opaqueMaterialBuckets_[objectPart.materialConfig].push_back(&objectPart);
	//	}
	//}
}

void InstanceManager::SpriteLayerManagement() {
	if (tile2DList_.empty()) return;

	/// スロットの基本値とステップ値、0から0.01ずつ減少
	float baseZ = 0.0f;
	const float slotStep = -0.01f;

	std::vector<SpriteInstance*> unlayered;
	std::vector<SpriteInstance*> layered;

	// position.zが-0.4f以下のスプライトとそれ以外で分ける
	for (auto* sprite : tile2DList_) {
		if (sprite->position.z <= -0.4f)
			layered.push_back(sprite);
		else
			unlayered.push_back(sprite);
	}

	/// slot設定

	/// stable_sortを使ってスロットし、Z値が同じ場合でも元の順序を保つ
	std::stable_sort(unlayered.begin(), unlayered.end(),
		[](auto* a, auto* b) { return a->position.z > b->position.z; });

	for (int i = 0; i < unlayered.size(); ++i) {
		unlayered[i]->position.z = baseZ + slotStep * static_cast<float>(i);
	}

	/// 最後に資料をspriteList_にまとめ直す
	tile2DList_.clear();
	tile2DList_.insert(tile2DList_.end(), unlayered.begin(), unlayered.end());
	tile2DList_.insert(tile2DList_.end(), layered.begin(), layered.end());
}