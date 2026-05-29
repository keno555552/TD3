#include "AnimationManager.h"
#include "kEngine.h"
#include "externals/nlohmann/json.hpp"
#include "Resource/ResourceManager.h"
#include <Camera/Camera.h>

AnimationManager::AnimationManager(kEngine* system) {
	system_ = system;
}

AnimationManager::~AnimationManager() {
}

std::vector<int> AnimationManager::LoadAnimation(const std::string& filePath) {

	/// まずリストを作る
	std::vector<int> result;

	/// Listに元々あるかどうか確認する,あったらそれをリストに作って返す
	for (int i = 0; i < animationDataList_.size(); ++i) {
		if (animationDataList_[i].filePath == filePath) {
			result.push_back(i);
		}
	}
	if (!result.empty()) return result;


	/// ResourceManagerにローディング/ハンドルを求む
	int modelDataHandle = system_->GetResourceManager()->ReadFile(filePath);
	std::weak_ptr<ModelData> modelData = ResourceManager::GetInstance()->GetModelData(modelDataHandle);

	for (int i = 0; i < modelData.lock()->animationList.size(); ++i) {
		AnimationClip clip{};
		clip.modelDataHandle = modelDataHandle;
		clip.modelDataPtr = modelData;
		clip.animationIndex = i;
		clip.filePath = filePath;
		animationDataList_.push_back(clip);
		result.push_back(int(animationDataList_.size() - 1));
	}

	if (result.empty()) {
		Logger::Log("[kEngine] AM :: Unknow Error.");
	}

	///ハンドルを返す
	return result;
}

void AnimationManager::UnitSetTime(int unitHandle, float time) {

	bool retFlag = CheckHaveHandle(unitHandle);
	if (!retFlag) return;

	unitList_[unitHandle]->SetTime(time);
}

int AnimationManager::TakeControlObject(int animationHandle, Object* object) {

	/// ハンドルが存在するか確認する
	if (!CheckHaveHandle(animationHandle)) {
		return -1;
	}

	/// nullcheck
	if (!object) {
		Logger::Log("[kError] AM::UnitSetTime() Invalid animation unit handle.");
		return -1;
	}

	/// animationHandleからアニメーションデータを読み込む
	auto& ani = animationDataList_[animationHandle];

	/// モデルデータが有効か確認する
	auto model = ani.modelDataPtr.lock();
	if (!model) {
		Logger::Log("[kError] AM::UnitSetTime() ModelData expired.");
		return -1;
	}

	/// Unitを作成して、アニメーションデータを読み込む
	unitList_[unitHandleCounter_] = std::make_unique<AnimationUnit>(system_);
	unitList_[unitHandleCounter_]->ReadAnimationData(model, ani.animationIndex);
	unitList_[unitHandleCounter_]->TakeControlObject(object);

	/// objectとUnitのHandleを紐づける
	objectToUnitHandleList_[object] = unitHandleCounter_;
	return unitHandleCounter_++;/// Unitのハンドルを返す,そしてカウンター＋1
}

int AnimationManager::GetControlObjectPartHandle(ObjectData* object) {

	/// nullcheck
	if (!object)return -1;

	/// objectがUnitに紐づいているか確認する
	auto it = objectToUnitHandleList_.find(object);
	if (it == objectToUnitHandleList_.end()) {
		return -1;
	}
	/// 紐づいているUnitのHandleを返す
	return it->second;
}

Object* AnimationManager::GetInstanceObjectByUnitHandle(int unitHandle) {
	if (unitList_.contains(unitHandle)) {
		return unitList_[unitHandle]->GetInstanceObject();
	}
	Logger::Log("[kError] AM::GetInstanceObjectByUnitHandle() Invalid animation unit handle.");
	return nullptr;
}

void AnimationManager::Update() {
	for (auto& [handle, unit] : unitList_) {
		unit->Update();
	}
}

Animation AnimationManager::LoadAnimationDataFromJson(const std::string& filePath) {
	//std::ifstream input(filePath);
	//if (!input.is_open()) {
	//	throw std::runtime_error("Failed to open file: " + filePath);
	//}
	//
	//nlohmann::json file;
	//input >> file;  // 讀取並解析 JSON
	//
	//// 基本資訊
	//
	//Animation result;
	//result.nodeList[0].animationNodeID = file["AnimationID"].get<int>();
	//
	//// KeyFrameList
	//for (const auto& kfJson : file["KeyFrameList"]) {
	//	KeyFrame kf;
	//	kf.index_ = kfJson["Index"].get<int>();
	//	kf.time_ = kfJson["Time"].get<float>();
	//	kf.animationType_ = static_cast<AnimationType>(kfJson["AnimationType"].get<int>());
	//	kf.easeRate_ = kfJson["EaseRate"].get<float>();
	//
	//	// MainPosition
	//	auto mp = kfJson["TransformData"]["MainPosition"];
	//	kf.transformData.mainPosition.transform.translate.x = mp["TranslateX"].get<float>();
	//	kf.transformData.mainPosition.transform.translate.y = mp["TranslateY"].get<float>();
	//	kf.transformData.mainPosition.transform.translate.z = mp["TranslateZ"].get<float>();
	//	kf.transformData.mainPosition.transform.rotate.x = mp["RotateX"].get<float>();
	//	kf.transformData.mainPosition.transform.rotate.y = mp["RotateY"].get<float>();
	//	kf.transformData.mainPosition.transform.rotate.z = mp["RotateZ"].get<float>();
	//	kf.transformData.mainPosition.transform.scale.x = mp["ScaleX"].get<float>();
	//	kf.transformData.mainPosition.transform.scale.y = mp["ScaleY"].get<float>();
	//	kf.transformData.mainPosition.transform.scale.z = mp["ScaleZ"].get<float>();
	//
	//	// ObjectParts
	//	for (const auto& partJson : kfJson["TransformData"]["ObjectPart"]) {
	//		ObjectPart part;
	//		part.transform.translate.x = partJson["TranslateX"].get<float>();
	//		part.transform.translate.y = partJson["TranslateY"].get<float>();
	//		part.transform.translate.z = partJson["TranslateZ"].get<float>();
	//		part.transform.rotate.x = partJson["RotateX"].get<float>();
	//		part.transform.rotate.y = partJson["RotateY"].get<float>();
	//		part.transform.rotate.z = partJson["RotateZ"].get<float>();
	//		part.transform.scale.x = partJson["ScaleX"].get<float>();
	//		part.transform.scale.y = partJson["ScaleY"].get<float>();
	//		part.transform.scale.z = partJson["ScaleZ"].get<float>();
	//
	//		kf.transformData.objectParts_.push_back(part);
	//	}
	//
	//	animationList->keyList.push_back(kf);
	//}
	//keyFrameList_.clear();
	//for (auto& kf : animationList->keyList) {
	//	KeyFrame newKf;
	//	newKf.index_ = kf.index_;
	//	newKf.time_ = kf.time_;
	//	newKf.animationType_ = kf.animationType_;
	//	newKf.easeRate_ = kf.easeRate_;
	//	newKf.transformData = kf.transformData;
	//	newKf.hitBox_ = {
	//		{ startPos_.x + ((EndPos_.x - startPos_.x) / mainTimer_->maxTime_) * kf.time_,
	//		  (float)config::GetClientHeight() - 10.0f + 5.0f },
	//		10.0f, 10.0f
	//	};
	//	keyFrameList_.push_back(newKf);
	//}
	//
	//ping_->objectParts_.clear();
	//for (const auto& kf : keyFrameList_) {
	//	ping_->CreateDefaultData();
	//	float Leight = EndPos_.x - startPos_.x;
	//	float timeParameter = kf.time_ / mainTimer_->maxTime_;
	//	ping_->objectParts_.back().materialConfig->textureHandle = pingTH_;
	//	ping_->objectParts_.back().transform.translate = {
	//		startPos_.x + Leight * timeParameter,
	//		(float)config::GetClientHeight() - 10.0f,
	//		100.0f
	//	};
	//	ping_->objectParts_.back().anchorPoint = { 5.0f,0.0f };
	//}
	return {};
}


bool AnimationManager::CheckHaveHandle(int animHandle) {

	/// ハンドルが存在するか確認する
	if (animHandle < 0 ||
		animHandle >= animationDataList_.size()) {
		Logger::Log("[kError] AM::CheckHaveHandle() Invalid animation handle.");
		return false;
	}

	/// すでに読み込んでいるファイルならばハンドルを返す
	if (animationDataList_[animHandle].modelDataPtr.expired()) {
		Logger::Log("[kError] AM::CheckHaveHandle() ModelData expired.");
		return false;
	}
	return true;
}