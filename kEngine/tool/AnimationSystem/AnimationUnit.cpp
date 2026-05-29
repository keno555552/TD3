#include "AnimationUnit.h"
#include "kEngine.h"
#include "externals/nlohmann/json.hpp"
#include <Camera/Camera.h>

AnimationUnit::AnimationUnit(kEngine* system) {
	system_ = system;

	nowTime_ = 0.0f;

	instanceObject_ = std::make_unique<Object>();
	instanceObject_->IntObject(system_);
}

AnimationUnit::~AnimationUnit() {}

AnimationUnit* AnimationUnit::ReadAnimationData(std::shared_ptr<ModelData> modelData, int animationIndex) {


	modelData_ = modelData;
	animationIndex_ = animationIndex;
	allMaxTime_ = modelData->animationList[animationIndex].duration;
	allStartTime_ = modelData->animationList[animationIndex].startTime;

	return this;
}

void AnimationUnit::TakeControlObject(Object* object) {

	if (!object) {
		Logger::Log("[kError] AS :: No such object.");
		return;
	}

	controlledObject_ = object;
	instanceObject_->CopyObject(controlledObject_);

	if (modelData_.lock()->skeleton.rootID >= 0) {

		haveSkeleton_ = true;

		instanceSkeleton_ = std::make_unique<Skeleton>(modelData_.lock()->skeleton);
		for (auto& j : instanceSkeleton_->jointList) {
			j.localMatrix = Identity();
			j.skeletonSpaceMatrix = Identity();
		}

	} else {
		haveSkeleton_ = false;
	}
	BindingAnimationNodeToObjectPart();

}

void AnimationUnit::Update() {

	/// ローカルポーズを更新する
	UpdateLocalPose();

	/// スケルトンがあれば続く
	if (!haveSkeleton_)return;

	/// skeleton行列を更新する
	UpdateLocalMatrix();
	UpdateGolbalMatrixAndFinalMatrix();

}

void AnimationUnit::SetTime(float time_) {

	/// 時間を制限内に収める
	nowTime_ = time_;
	if (nowTime_ < allStartTime_)nowTime_ = allStartTime_;
	if (nowTime_ > allMaxTime_)nowTime_ = allMaxTime_;

}

void AnimationUnit::UpdateLocalPose() {

	/// アニメーションがなければ終わる
	if (!modelData_.lock()) {
		isObjectChange_ = false;
		return;
	}

	/// アニメーションとobject partの対応がなければ終わる
	if (animationBindings.animToObject.empty()) {
		isObjectChange_ = false;
		return;
	}

	/// まずはアニメーションをとる
	auto& anim = modelData_.lock()->animationList[animationIndex_];

	/// nodeごとに、object partのアニメーションを更新する
	for (size_t i = 0; i < anim.nodeList.size(); i++) {

		if(i >= animationBindings.animToObject.size()) continue;

		int objIndex = animationBindings.animToObject[i];

		// nodeに対応するobject partがない場合はスキップ
		if (objIndex < 0 || objIndex >= instanceObject_->objectParts_.size()) {
			continue;
		}

		auto& node = anim.nodeList[i];
		auto& part = instanceObject_->objectParts_[objIndex].transform;

		part.aniScale = MakeTimeValue(node.scaleList, nowTime_);
		part.aniRotate = MakeTimeValue(node.rotateList, nowTime_);
		part.aniTranslate = MakeTimeValue(node.translationList, nowTime_);

		part.isAnimated = true;
	}
	isObjectChange_ = true;
}

void AnimationUnit::UpdateLocalMatrix() {

	auto& anim = modelData_.lock()->animationList[animationIndex_];
	for (size_t i = 0; i < anim.nodeList.size(); i++) {

		int objIndex = animationBindings.animToObject[i];

		/// nodeに対応するobject partがない場合はスキップ
		if (objIndex < 0 || objIndex >= instanceObject_->objectParts_.size()) {
			continue;
		}

		/// jointIndexを探す（iと同じとは限らない）
		const std::string& nodeName = anim.nodeList[i].name;
		auto it = instanceSkeleton_->jointMap.find(nodeName);
		if (it == instanceSkeleton_->jointMap.end()) {
			Logger::Log("[kWarning] No joint found for animation node: " + nodeName);
			continue;
		}
		int jointIndex = it->second;

		/// ローカル行列を更新する
		auto& part = instanceObject_->objectParts_[objIndex].transform;
		instanceSkeleton_->jointList[jointIndex].localMatrix =
			MakeAffineMatrix(
				part.aniScale,
				part.aniRotate,
				part.aniTranslate
			);

	}
}

void AnimationUnit::UpdateGolbalMatrixAndFinalMatrix() {

	/// skeletonをとる
	auto& joints = instanceSkeleton_->jointList;
	auto& bindJoints = modelData_.lock()->skeleton.jointList;

	/// Jointを沿ってglobal行列を更新する
	for (size_t i = 0; i < joints.size(); i++) {

		Joint& joint = joints[i];

		// 親のglobal行列と自分のlocal行列をかける
		if (!joint.parentID.has_value()) {
			// root
			joint.skeletonSpaceMatrix = joint.localMatrix;
		} else {
			int parentIndex = joint.parentID.value();
			joint.skeletonSpaceMatrix = joints[parentIndex].skeletonSpaceMatrix * joint.localMatrix;
		}

		// 2. finalMatrixの更新
		joint.skeletonSpaceMatrixInvers = bindJoints[i].skeletonSpaceMatrix.Inverse();
		joint.finalMatrix = joint.skeletonSpaceMatrix * joint.skeletonSpaceMatrixInvers;
	}
}

float AnimationUnit::ChangeEasing(AnimationType type, float t, float rate) {

	switch (type) {
	case AnimationType::LINEARITY:
		return linearity(0, 1, t);
	case AnimationType::EASY_IN:
		return easyIn(0, 1, t, rate);
	case AnimationType::EASY_OUT:
		return easyOut(0, 1, t, rate);
	case AnimationType::EASY_IN_OUT:
		return easyInOut(0, 1, t, rate);
	case AnimationType::EASY_IN_BACK:
		return easyInBack(0, 1, t, rate);
	case AnimationType::EASY_OUT_BACK:
		return easyOutBack(0, 1, t, rate);
	}
	return 0;
}

void AnimationUnit::BindingAnimationNodeToObjectPart() {

	auto& anim = modelData_.lock()->animationList[animationIndex_];
	if (!modelData_.lock()) return;

	animationBindings.animToObject.resize(anim.nodeList.size(), -1);

	for (int i = 0; i < anim.nodeList.size(); i++) {

		const std::string& nodeName = anim.nodeList[i].name;

		auto target = std::find_if(
			controlledObject_->objectParts_.begin(),
			controlledObject_->objectParts_.end(),
			[&](const ObjectPart& object) {
				return nodeName == object.name;
			}
		);

		/// partのIndexをとる
		int partIndex = (int)std::distance(controlledObject_->objectParts_.begin(), target);

		/// アニメーションのノードが見つかったら、partIndexとnodeIndexを紐づける
		if (target != controlledObject_->objectParts_.end()) {

			animationBindings.animToObject[i] = partIndex;
		} else {
			Logger::Log("[kWarning] AS_Unit :: No object part found for animation node: " + nodeName);
			animationBindings.animToObject[i] = -1; // No animation
		}
	}
}