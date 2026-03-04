#include "Sprite.h"


void SimpleSprite::CreateDefaultData() {
	modelHandle_ = 0;

	if(mainPosition.materialConfig == nullptr){
		mainPosition.materialConfig = std::make_shared<MaterialConfig>();
		InitMaterialConfig(mainPosition.materialConfig.get());
	}

	SimpleSpritePart newObjectPart;
	newObjectPart.materialConfig = std::make_shared<MaterialConfig>();
	InitMaterialConfig(newObjectPart.materialConfig.get());
	newObjectPart.materialConfig->enableLighting = false;
	newObjectPart.materialConfig->MakeUVMatrix();
	newObjectPart.materialConfig->lightModelType = LightModelType::Sprite2D;

	objectParts_.push_back(newObjectPart);
}

void SimpleSprite::IntObject(kEngine* system) {
	system_ = system;
}

void SimpleSprite::Update(Camera* camera) {
	camera;

	//for (auto& ptr : objectParts_) {
	//	ptr.materialConfig->MakeUVMatrix();
	//}
	//
	//if (mainPosition.materialConfig != nullptr) {
	//	mainPosition.materialConfig->MakeUVMatrix();
	//}
	//mainPosition.worldAnchorPoint = mainPosition.anchorPoint;
	//
	//Transform followWorld;
	//if (followObject_ != nullptr) {
	//	followWorld.scale = { 1.0f, 1.0f ,1.0f };   // 預設單位縮放
	//	followWorld.rotate = { 0.0f, 0.0f, 0.0f };           // 預設不旋轉
	//	followWorld.translate = { 0.0f, 0.0f, 0.0f };   // 預設不平移
	//
	//	SpriteData* tempFollow = followObject_;
	//	while (tempFollow != nullptr) {
	//		followWorld.scale *= tempFollow->mainPosition.transform.scale;
	//		followWorld.rotate += tempFollow->mainPosition.transform.rotate;
	//		followWorld.translate += tempFollow->mainPosition.transform.translate;
	//	
	//		tempFollow = tempFollow->followObject_;
	//	}
	//} else {
	//	followWorld.scale = { 1.0f, 1.0f ,1.0f };   
	//	followWorld.rotate = { 0.0f, 0.0f, 0.0f };     
	//	followWorld.translate = { 0.0f, 0.0f, 0.0f }; 
	//}
	//
	//
	//for (auto& part : objectParts_) {
	//
	//	part.worldTransform.scale = part.transform.scale * mainPosition.transform.scale * followWorld.scale;
	//	part.worldTransform.rotate = part.transform.rotate + mainPosition.transform.rotate + followWorld.rotate;
	//	part.worldTransform.translate = part.transform.translate + mainPosition.transform.translate + followWorld.translate;
	//
	//	part.materialConfig->MakeUVMatrix();
	//
	//	part.worldAnchorPoint = mainPosition.worldAnchorPoint + part.anchorPoint;
	//}
	//
}

void SimpleSprite::Draw() {
	system_->Draw2D(this);
}

Vector3 SimpleSprite::TransRotation(float theat) {
	return Vector3(0, 0, theat);
}

Vector3 SimpleSprite::TransTransform(Vector2 Pos) {
	return Vector3(Pos.x, Pos.y, 0);
}

float SimpleSprite::TransLayerDepth(int layer) {
	return layer * layerDepth_Sprite;
}
