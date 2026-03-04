#include "Object.h"
#include "Vector3.h"
void Object::CreateDefaultData() {
	modelHandle_ = 0;

	ObjectPart newObjectPart;
	newObjectPart.materialConfig = std::make_shared<MaterialConfig>();
	InitMaterialConfig(newObjectPart.materialConfig.get());
	newObjectPart.materialConfig->lightModelType = LightModelType::HalfLambert;
	newObjectPart.materialConfig->enableLighting = true;

	objectParts_.push_back(newObjectPart);
}

void Object::IntObject(kEngine* system) {
	system_ = system;
}

void Object::Update(Camera* camera) {
	//ObjectPart mainObjectPart = mainPosition;
	//
	//// 父物件矩陣
	//Matrix4x4 parentMatrix = Identity();
	//
	//if (followObject_ != nullptr) {
	//	if (isBillboard_) {
	//		// Billboard 只繼承平移，不繼承旋轉
	//		parentMatrix = MakeTranslateMatrix(followObject_->transform.translate);
	//		// 如果需要縮放，可以加上：
	//		parentMatrix = parentMatrix * MakeScaleMatrix4x4(followObject_->transform.scale) ;
	//	} else {
	//		// 一般物件才繼承完整的 affine
	//		parentMatrix = MakeAffineMatrix(
	//			followObject_->transform.scale,
	//			followObject_->transform.rotate,
	//			followObject_->transform.translate
	//		);
	//	}
	//}
	//
	//
	//// 主物件矩陣
	//Matrix4x4 objectMainMatrix = MakeAffineMatrix(
	//	mainObjectPart.transform.scale,
	//	mainObjectPart.transform.rotate,
	//	mainObjectPart.transform.translate
	//);
	//
	//// Billboard處理
	//if (isBillboard_) {
	//	Transform camTransform = camera->GetTransform();
	//
	//	//カメラのforward向量
	//	float cosY = cosf(camTransform.rotate.y);
	//	float sinY = sinf(camTransform.rotate.y);
	//	float cosX = cosf(camTransform.rotate.x);
	//	float sinX = sinf(camTransform.rotate.x);
	//
	//	// Forward (Z軸)
	//	Vector3 camForward = {
	//		sinY * cosX,
	//		-sinX,
	//		cosY * cosX
	//	};
	//
	//	// Right (X軸)
	//	Vector3 camRight = {
	//		cosY,
	//		0.0f,
	//		-sinY
	//	};
	//
	//	// Up (Y軸)
	//	Vector3 camUp = {
	//		sinY * sinX,
	//		cosX,
	//		cosY * sinX
	//	};
	//
	//	Matrix4x4 billboardMatrix = {
	//		camRight.x, camRight.y,camRight.z,0,
	//		camUp.x,camUp.y, camUp.z, 0,
	//		camForward.x,camForward.y, camForward.z, 0,
	//		mainObjectPart.transform.translate.x,mainObjectPart.transform.translate.y,mainObjectPart.transform.translate.z,1
	//	};
	//
	//	// 縮放計算
	//	objectMainMatrix = billboardMatrix * MakeScaleMatrix4x4(mainObjectPart.transform.scale);
	//}
	//
	//Matrix4x4 objectWorldMatrix = objectMainMatrix * parentMatrix;
	//mainPosition.transformationMatrix = camera->transformationMatrixTransform(objectWorldMatrix);
	//
	//// 子物件處理
	//for (auto& part : objectParts_) {
	//	Matrix4x4 objectParentMatrix = Identity();
	//	if (part.parentPart != nullptr) {
	//		objectParentMatrix = MakeAffineMatrix(
	//			part.parentPart->transform.scale,
	//			part.parentPart->transform.rotate,
	//			part.parentPart->transform.translate
	//		);
	//	}
	//
	//	// Billboard 子物件：XY 旋轉を0にする
	//	if (isBillboard_) {
	//		part.transform.rotate.x = 0.0f;
	//		part.transform.rotate.y = 0.0f;
	//	}
	//
	//	Matrix4x4 localMatrix = MakeAffineMatrix(
	//		part.transform.scale,
	//		part.transform.rotate,
	//		part.transform.translate
	//	);
	//
	//	Matrix4x4 worldMatrix = localMatrix * objectParentMatrix * objectWorldMatrix;
	//	part.transformationMatrix = camera->transformationMatrixTransform(worldMatrix);
	//	part.materialConfig->MakeUVMatrix();
	//}
}


void Object::Draw() {
	system_->Draw3D(this);
}

void Object::CreateModelData(int modelHandle) {
	modelHandle_ = modelHandle;
	int numOfPart = system_->GetMutiModelNum(modelHandle_);

	mainPosition.materialConfig = std::make_shared<MaterialConfig>();
	InitMaterialConfig(mainPosition.materialConfig.get());

	objectParts_.clear();

	for (int i = 0; i < numOfPart; i++) {
		ObjectPart newObjectPart;
		newObjectPart.materialConfig = std::make_shared<MaterialConfig>();
		InitMaterialConfig(newObjectPart.materialConfig.get());
		newObjectPart.materialConfig->lightModelType = LightModelType::HalfLambert;
		newObjectPart.materialConfig->enableLighting = true;
		newObjectPart.materialConfig->useModelTexture = true;
		newObjectPart.materialConfig->textureHandle = system_->GetModelTextureHandle(modelHandle, i);
		objectParts_.push_back(newObjectPart);
	}

}
// �Փ˂����I�u�W�F�N�g�̃|�C���^�[��n��
Object* Object::Collision(const std::vector<Object*>& obj)
{
	return nullptr;
}
// �Փ˂����Ƃ��߂荞�܂Ȃ��ꍇ
void Object::Conflict(const std::vector<Object*>& obj)
{
}
Object* Object::Collision(const Object* obj)
{
	return nullptr;
}
AABB Object::GetAABB(const Object* obj) const
{
  
	//// ���[���h���W�̒��S�ʒu��v�Z
	Vector3 pos = obj->mainPosition.transform.translate ;
 //   
 //   // ���C��: �X�P�[���̍����i�e�̃X�P�[�� �~ �q�̃X�P�[���j
    Vector3 scale = obj->mainPosition.transform.scale;
 
    
   //Vector3 pos = obj->objectParts_[0].transform.translate;
   // 
   // // �I�u�W�F�N�g�̃X�P�[���i���E�����E���s���̔����Ɖ���j
   // // �����f����1x1x1�̗����̂ł���O��ł��B
   // // ���f�����傫���ꍇ�́AScale * 2.0f �Ȃǒ������Ă��������B
   // Vector3 scale = obj->objectParts_[0].transform.scale ; 

    // 半径（Half extents）にするため 0.5倍
    scale *= 0.5f;

    AABB box;
    box.min = { pos.x - scale.x, pos.y - scale.y, pos.z - scale.z };
    box.max = { pos.x + scale.x, pos.y + scale.y, pos.z + scale.z };
    return box;
}
void Object::CopyObject(Object* target) {
	if (target == nullptr)return;
	mainPosition = target->mainPosition;
	followObject_ = target->followObject_;
	for (auto& ptr : target->objectParts_) {
		CreateDefaultData();
		objectParts_.back().transform = ptr.transform;
		objectParts_.back().materialConfig = ptr.materialConfig;
		objectParts_.back().parentPart = ptr.parentPart;
	}
}

