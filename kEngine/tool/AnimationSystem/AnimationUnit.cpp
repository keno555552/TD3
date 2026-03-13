#include "AnimationUnit.h"

AnimationUnit::AnimationUnit(kEngine* system) {
	system_ = system;

	time_ = std::make_unique<Timer>();
	time_->Init0(1.0f, system_->GetTimeManager());

	instanceObject_ = std::make_unique<Object>();
	instanceObject_->IntObject(system_);

}

AnimationUnit::~AnimationUnit() {
	time_.reset();
	instanceObject_.reset();
}

void AnimationUnit::ReadAnimationData(AnimationObjectData* animationData) {
	animationData_ = animationData;
	if (!animationData->keyList.empty()) {
		allMaxTime_ = animationData->keyList.back().time_;
		allStartTime_ = animationData->keyList.front().time_;
		ResetTimer();
	}
}



void AnimationUnit::SetTime(float time_) {
	nowTime_ = time_;
	allStartTime_ = animationData_->keyList.front().time_;
	allMaxTime_ = animationData_->keyList.back().time_;
	if (nowTime_ < allStartTime_)nowTime_ = allStartTime_;
	if (nowTime_ > allMaxTime_)nowTime_ = allMaxTime_;
}

void AnimationUnit::TakeControlObject(Object* object) {
	if (object == nullptr)return;
	if (!CheckObjectNumMeet(object))return;
	controlledObject_ = object;
	instanceObject_->CopyObject(controlledObject_);
}

void AnimationUnit::RelistControlObject() {
	controlledObject_ = nullptr;
}

void AnimationUnit::Update(Camera* camera) {
	TunningTime();

	UpdateInstanceObject();

	ControlleObject(camera);

}

void AnimationUnit::KariDraw() {
#ifdef USE_IMGUI
	ImguiPart();
#endif
}

bool AnimationUnit::CheckObjectNumMeet(Object* target) {
	if (target->objectParts_.size() != animationData_->keyList[0].transformData.objectParts_.size())return false;
	return true;
}

void AnimationUnit::TunningTime() {
	for (size_t i = 0; i + 1 < animationData_->keyList.size(); ++i) {
		const auto& current = animationData_->keyList[i];
		const auto& next = animationData_->keyList[i + 1];

		if (nowTime_ >= current.time_ && nowTime_ < next.time_) {
			usingKeyFrameIndex_ = static_cast<int>(i);
			usingStartTime_ = current.time_;
			usingEndTime_ = next.time_;

			break;
		}
	}

	if (nowTime_ >= animationData_->keyList.back().time_) {
		usingKeyFrameIndex_ = static_cast<int>(animationData_->keyList.size() - 1);
		usingStartTime_ = animationData_->keyList.back().time_;
		usingEndTime_ = allMaxTime_;
	}

	time_->maxTime_ = usingEndTime_ - usingStartTime_;
	time_->parameter_ = nowTime_ - usingStartTime_;
	if (usingEndTime_ == usingStartTime_)time_->parameter_ = usingEndTime_;
}


void AnimationUnit::ResetTimer() {
	usingStartTime_ = 0;
	usingEndTime_ = animationData_->keyList.front().time_;
}

void AnimationUnit::UpdateInstanceObject() {
	float T = ChangeEasing(AnimationType::LINEARITY);
	int index = usingKeyFrameIndex_;
	if (index + 1 >= animationData_->keyList.size())index -= 1;
	KeyFrame usingKeyFrame = animationData_->keyList[index + 1];
	KeyFrame frontKeyFrame = animationData_->keyList[index];

	if (instanceObject_ != nullptr) {
		T = ChangeEasing(usingKeyFrame.animationType_, (float)usingKeyFrame.easeRate_);

		instanceObject_->mainPosition.transform = {
			frontKeyFrame.transformData.mainPosition.transform.scale * (1 - T) + usingKeyFrame.transformData.mainPosition.transform.scale * T,
			frontKeyFrame.transformData.mainPosition.transform.rotate * (1 - T) + usingKeyFrame.transformData.mainPosition.transform.rotate * T,
			frontKeyFrame.transformData.mainPosition.transform.translate * (1 - T) + usingKeyFrame.transformData.mainPosition.transform.translate * T
		};

		int partNum = 0;
		for (auto& part : instanceObject_->objectParts_) {
			instanceObject_->objectParts_[partNum].transform = {
				frontKeyFrame.transformData.objectParts_[partNum].transform.scale * (1 - T) + usingKeyFrame.transformData.objectParts_[partNum].transform.scale * T,
				frontKeyFrame.transformData.objectParts_[partNum].transform.rotate * (1 - T) + usingKeyFrame.transformData.objectParts_[partNum].transform.rotate * T,
				frontKeyFrame.transformData.objectParts_[partNum].transform.translate * (1 - T) + usingKeyFrame.transformData.objectParts_[partNum].transform.translate * T
			};
			partNum++;
		}
	}
}

float AnimationUnit::ChangeEasing(AnimationType type, float R) {
	if (time_->parameter_ == 0) return 0;
	float T{};
	switch (type) {
	case AnimationType::LINEARITY:
		return time_->linearity();
	case AnimationType::EASY_IN:
		return time_->easyIn(R);
	case AnimationType::EASY_OUT:
		return time_->easyOut(R);
	case AnimationType::EASY_IN_OUT:
		return time_->easyInOut(R);
	case AnimationType::EASY_IN_BACK:
		return time_->easyInBack(R);
	case AnimationType::EASY_OUT_BACK:
		return time_->easyOutBack(R);
	}
	return 0;
}

void AnimationUnit::ControlleObject(Camera* camera) {

	Matrix4x4 parentMatrix = Identity();
	if (controlledObject_->followObject_ != nullptr) {
		parentMatrix = MakeAffineMatrix(
			controlledObject_->followObject_->transform.scale,
			controlledObject_->followObject_->transform.rotate,
			controlledObject_->followObject_->transform.translate
		);
	}

	Matrix4x4 objectMainMatrix = MakeAffineMatrix(
		controlledObject_->mainPosition.transform.scale,
		controlledObject_->mainPosition.transform.rotate,
		controlledObject_->mainPosition.transform.translate
	);

	Matrix4x4 objectAnimateMainMatrix = MakeAffineMatrix(
		instanceObject_->mainPosition.transform.scale,
		instanceObject_->mainPosition.transform.rotate,
		instanceObject_->mainPosition.transform.translate
	);
	Matrix4x4 objectWorldMatrix = objectMainMatrix * objectAnimateMainMatrix * parentMatrix;
	controlledObject_->mainPosition.transformationMatrix = camera->transformationMatrixTransform(objectWorldMatrix);

	int partNum = 0;
	for (auto& part : controlledObject_->objectParts_) {
		Matrix4x4 objectParentMatrix = Identity();
		if (part.parentPart != nullptr) {
			objectParentMatrix = MakeAffineMatrix(
				part.parentPart->transform.scale,
				part.parentPart->transform.rotate,
				part.parentPart->transform.translate
			);
		}

		Matrix4x4 localMatrix = MakeAffineMatrix(
			instanceObject_->objectParts_[partNum].transform.scale,
			instanceObject_->objectParts_[partNum].transform.rotate,
			instanceObject_->objectParts_[partNum].transform.translate
		);

		Matrix4x4 worldMatrix = localMatrix * objectParentMatrix * objectWorldMatrix;
		part.transformationMatrix = camera->transformationMatrixTransform(worldMatrix);
		part.materialConfig->MakeUVMatrix();
		partNum++;
	}
}

void AnimationObjectData::SetSimpleObject(const Object& obj) {
	Object obje = obj;
	SimpleObject.CopyObject(&obje);
}

void AnimationObjectData::AddKeyFrame(float time_) {
	KeyFrame keyFrame;
	if (time_ > 0.0f)keyFrame.time_ = time_;
	keyFrame.index_ = (int)keyList.size();
	keyFrame.animationType_ = AnimationType::LINEARITY;
	keyFrame.easeRate_ = 0.0f;
	keyFrame.transformData.CopyObject(&SimpleObject);

	keyList.push_back(keyFrame);
}

#ifdef USE_IMGUI
void AnimationUnit::ImguiPart() {
//	ImGui::Begin("AU0");
//	ImGui::Text("NowTime: %.02f", nowTime_);
//	ImGui::Text("parameterTime: %.02f", time_->parameter_);
//	ImGui::Text("usingStartTime: %.02f", usingStartTime_);
//	ImGui::Text("usingEndTime: %.02f", usingEndTime_);
//	ImGui::Text("allMaxTime: %.02f", allMaxTime_);
//	ImGui::Text("allStartTime: %.02f", allStartTime_);
//	ImGui::Text("controlledObjectPos.x: %.02f", controlledObject_->mainPosition.transform.translate.x);
//	ImGui::Text("instanceObjectPos.x: %.02f", instanceObject_->mainPosition.transform.translate.x);
//	ImGui::End();
}
#endif