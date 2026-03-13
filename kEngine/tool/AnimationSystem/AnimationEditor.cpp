#include "AnimationEditor.h"

AnimationEditor::AnimationEditor(kEngine* system) {
	system_ = system;
	SetupAnimationEditor();

	/// =============== モデル設定 ===============///
	std::string modelPath = "resources/TemplateResource/object/";
	targetModelHandle_ = system_->SetModelObj(modelPath + "player/player.obj");

	targetModel_ = std::make_unique<Object>();
	targetModel_->IntObject(system_);
	targetModel_->CreateModelData(targetModelHandle_);
	targetModel_->objectParts_[2].parentPart = &targetModel_->objectParts_[1];
	SetUsingModel(targetModel_.get());


	/// ================= Test ================///

	/// ================ システム最終設定 ================///
	SetupAnimationEditorEnd();
	animationUnit_->ReadAnimationData(&animationList_[0]);
	animationUnit_->TakeControlObject(targetModel_.get());
}

AnimationEditor::~AnimationEditor() {
	system_->DestroyCamera(camera_);

	mainNeedle_.reset();
	mainTimeBar_.reset();
	markerStartEnd_.reset();
	marker10_.reset();
	marker02_.reset();
	ping_.reset();
	instanceModel_.reset();

	mainTimer_.reset();
	skydome_.reset();

	animationUnit_.reset();
}

void AnimationEditor::Update() {
	UIUpdate();

	/// =============== モデル更新 =================///

	//targetModel_->Update(camera_);
	//animationUnit_->ReadAnimationData(animationList_[0]);
	animationUnit_->SetTime(mainTimer_->parameter_);
	animationUnit_->Update(camera_);
}

void AnimationEditor::Draw() {

	UIDraw();

	targetModel_->Draw();

	animationUnit_->KariDraw();

#ifdef USE_IMGUI
	ImguiPart();
#endif

}

void AnimationEditor::SetUsingModel(Object* model) {
	if (model == nullptr)return;
	choosingModel_ = model;
	instanceModel_ = std::make_unique<Object>();
	instanceModel_->IntObject(system_);
	instanceModel_->CopyObject(choosingModel_);
}


void AnimationEditor::SetChooseModel(Object* model) {
	if (model == nullptr)return;
	if (keyFrameModel_ != nullptr) keyFrameModel_ = nullptr;
	keyFrameModel_ = model;
}


void AnimationEditor::SetupAnimationEditor() {

	/// ========== 定数定義 ==========///
	float windowWidth = (float)config::GetClientWidth();
	float windowHeight = (float)config::GetClientHeight();

	/// ============= 初期カラー設定 ==============///
	markerColor2_ = { 42.0f,42.0f,42.0f,255.0f };
	markerColor2_.ColorBy1();

	/// ============ カメラ設定 ============///
	camera_ = system_->CreateCamera();
	camera_->Move({ 0.0f,0.5f,-10.0f });
	camera_->SetDefaultTransform(camera_->GetTransform());

	/// ======= タイマー設定 =======///
	mainTimer_ = std::make_unique<Timer>();
	mainTimer_->Init0(startMaxTime_, system_->GetTimeManager());
	mainTimer_->parameter_ = startMinTime_;

	/// ===== テキスチャローディング ===///
	std::string mainPath = "resources/TemplateResource/texture/animationEditor/";

	mainTimeBarTH_ = system_->LoadTexture(mainPath + "mainTimeBar.png");
	mainNeedleTH_ = system_->LoadTexture(mainPath + "mainNeedle.png");
	markerTH_ = system_->LoadTexture(mainPath + "marker.png");
	pingTH_ = system_->LoadTexture(mainPath + "ping.png");
	pingFirstTH_ = system_->LoadTexture(mainPath + "pingFirst.png");
	pingLastTH_ = system_->LoadTexture(mainPath + "pingLast.png");

	std::string modelPath = "resources/TemplateResource/object/";
	Skydome_modelHandle_ = system_->SetModelObj(modelPath + "animationEditor/studioBG/studioBG.obj");

	/// ========= skydome設定 ========///
	skydome_ = std::make_unique<Object>();
	skydome_->IntObject(system_);
	skydome_->CreateModelData(Skydome_modelHandle_);
	skydome_->objectParts_[0].materialConfig->enableLighting = false;

	/// ========= sprite設定 ========///
	mainTimeBar_->IntObject(system_);
	mainTimeBar_->CreateDefaultData();
	mainTimeBar_->objectParts_[0].materialConfig->textureHandle = mainTimeBarTH_;
	mainTimeBar_->objectParts_[0].transform.translate = { 0.0f, windowHeight - 50.0f,0.0f };
	mainTimeBar_->Update(nullptr);

	mainNeedle_->IntObject(system_);
	mainNeedle_->CreateDefaultData();
	mainNeedle_->objectParts_[0].materialConfig->textureHandle = mainNeedleTH_;
	mainNeedle_->objectParts_[0].transform.translate = { startPos_.x, startPos_.y,0.0f };
	mainNeedle_->Update(nullptr);

	markerStartEnd_->IntObject(system_);
	markerStartEnd_->CreateDefaultData();
	markerStartEnd_->CreateDefaultData();
	markerStartEnd_->objectParts_[0].materialConfig->textureHandle = markerTH_;
	markerStartEnd_->objectParts_[0].transform.translate = { startPos_.x, startPos_.y,0.0f };
	markerStartEnd_->objectParts_[0].materialConfig->textureColor = markerColor_;
	markerStartEnd_->objectParts_[1].materialConfig->textureHandle = markerTH_;
	markerStartEnd_->objectParts_[1].transform.translate = { EndPos_.x, EndPos_.y,0.0f };
	markerStartEnd_->objectParts_[1].materialConfig->textureColor = markerColor2_;
	markerStartEnd_->Update(nullptr);

	marker10_->IntObject(system_);
	marker02_->IntObject(system_);
	AdjustMarker();
	marker10_->Update(nullptr);
	marker02_->Update(nullptr);

	ping_->IntObject(system_);

	/// ======= ヒットボックス設定 =======///
	mainTimeBarHitBox_.midPos = { windowWidth / 2.0f, windowHeight - (50.0f / 2.0f) };
	mainTimeBarHitBox_.width = windowWidth - 100.0f;
	mainTimeBarHitBox_.height = 50.0f;

}

void AnimationEditor::SetupAnimationEditorEnd() {

	/// ======= デイタ設定 =======///
	AnimationObjectData testData{};
	testData.SetSimpleObject(*targetModel_);
	animationList_.push_back({ testData });
	animationUnit_ = std::make_unique<AnimationUnit>(system_);


	/// ======= 最初のピン設定 =======///
	CreateKeyFrame(startMinTime_);
	CreateKeyFrame(startMaxTime_);
}

void AnimationEditor::UIUpdate() {
	/// =============== mouse調整 ================///
	mouseMovement();

	/// ============= タイマーロジック =============///
	TimerPart();

	/// ============= バー更新ロジック =============///
	NeedlePart();
	MarkerPart();
	PingPart();

	KeyFrameTurnning();

	/// ============= 3D基本とカメラ更新 =============///
	skydome_->Update(camera_);
	camera_->Update();
}

void AnimationEditor::UIDraw() {
	skydome_->Draw();

	mainTimeBar_->Draw();

	markerStartEnd_->Draw();
	marker10_->Draw();
	marker02_->Draw();

	ping_->Draw();

	mainNeedle_->Draw();
}



void AnimationEditor::mouseMovement() {
	if (system_->GetMouseTriggerOn(0)) {
		Vector2 mousePos =  system_->GetMousePosVector2();
		for (int i = 0; i < keyFrameList_.size(); i++) {
			if (CrashDecisionHitBoxPointBool(&keyFrameList_[i].hitBox_, mousePos)) {
				pickedKeyFrame_ = keyFrameList_[i].index_;
				selectedKeyFrame_ = keyFrameList_[i].index_;
				isPickKeyFrame_ = true;
				isChooseKeyFrame_ = true;
				return;
			}
		}
	}
	if (system_->GetMouseTriggerOn(1)) {
		isChooseKeyFrame_ = false;
		selectedKeyFrame_ = -1;
	}
	if (system_->GetMouseIsPush(0)) {
		Vector2 mousePos = system_->GetMousePosVector2();
		if (!isPickKeyFrame_) {
			if (CrashDecisionHitBoxPointBool(&mainTimeBarHitBox_, mousePos)) {
				float Leight = EndPos_.x - startPos_.x;
				float nowLeight = mousePos.x - startPos_.x;
				float timeParameter;
				if (nowLeight != 0)timeParameter = (nowLeight / Leight) * mainTimer_->maxTime_;
				else timeParameter = 0.0f;
				if (timeParameter >= 0.0f && timeParameter <= mainTimer_->maxTime_) {
					mainTimer_->parameter_ = timeParameter;
				}
				return;
			}
		}
	}

	if (system_->GetMouseTriggerOff(0)) {
		isPickKeyFrame_ = false;
		pickedKeyFrame_ = -1;
		return;
	}
}

void AnimationEditor::TimerPart() {
	if (system_->GetTriggerOn(DIK_SPACE)) {
		if (isPlay_) {
			isPlay_ = false;
		} else {
			isPlay_ = true;
		}
	}

	if (isPlay_) {
		if (!isLoop_) {
			mainTimer_->ToMix();
		} else {
			mainTimer_->foreverUp();
		}
	}
}

void AnimationEditor::NeedlePart() {
	while (true) {
		if (mainTimer_->parameter_ == 0) {
			NowPos_ = startPos_;
			break;
		} else if (mainTimer_->parameter_ >= mainTimer_->maxTime_) {
			NowPos_ = EndPos_;
			break;
		} else {
			NowPos_.x = mainTimer_->linearity(startPos_.x, EndPos_.x);
			NowPos_.y = mainTimer_->linearity(startPos_.y, EndPos_.y);
			break;
		}
	}
	mainNeedle_->objectParts_[0].transform.translate = { NowPos_.x, NowPos_.y,0.0f };
	mainNeedle_->Update(nullptr);
}

void AnimationEditor::MarkerPart() {
	AdjustMarker();


	if (markerColor_ != saveMarkerColor_) {
		saveMarkerColor_ = markerColor_;
		for (auto& obj : marker10_->objectParts_) {
			obj.materialConfig->textureColor = markerColor_;
		}
	}

	if (markerColor2_ != saveMarkerColor2_) {
		saveMarkerColor2_ = markerColor2_;
		for (auto& obj : marker02_->objectParts_) {
			obj.materialConfig->textureColor = markerColor2_;
		}
	}

	markerStartEnd_->objectParts_[0].materialConfig->textureColor = markerColor_;
	markerStartEnd_->objectParts_[1].materialConfig->textureColor = markerColor_;

	markerStartEnd_->Update(nullptr);
	marker10_->Update(nullptr);
	marker02_->Update(nullptr);
}

void AnimationEditor::PingPart() {

	/// ============ ピン移動ロジック ============///
	if (!keyFrameList_.empty()) {
		if (pickedKeyFrame_ >= 0) {
			if (selectedKeyFrame_ < keyFrameList_.size()) {

				/// キーフレームリストから選んでるキーフレームを探す
				auto iterator = std::find_if(keyFrameList_.begin(), keyFrameList_.end(),
					[this](const KeyFrame& kf) {
						return kf.index_ == selectedKeyFrame_;
					});
				KeyFrame& listKeyFrame = *iterator;

				int index = int(iterator - keyFrameList_.begin());

				/// 指定されたキーフレームのピンを取得
				SpritePart& pingSprite = ping_->objectParts_[index];

				/// ピン移動処理
				float Leight = EndPos_.x - startPos_.x;
				if (isChooseKeyFrame_) {
					Vector2 mousePos = system_->GetMousePosVector2();
					float nowLeight = mousePos.x - startPos_.x - 5;
					if (nowLeight < 0) nowLeight = 0;
					if (nowLeight > Leight) nowLeight = Leight;
					float timeParameter = nowLeight / Leight;

					listKeyFrame.time_ = timeParameter * mainTimer_->maxTime_;
				}

				/// ピン位置更新
				float time = listKeyFrame.time_ / mainTimer_->maxTime_;
				if (time < 0)time = 0;
				if (time > 1)time = 1;
				pingSprite.transform.translate.x = startPos_.x + Leight * time + 5;
				listKeyFrame.hitBox_.midPos.x = pingSprite.transform.translate.x;
			}
		}
	}

	SortKeyFrame();
	AdjuctKeyFrameTexture();

	ping_->Update(camera_);
}

void AnimationEditor::AdjustMarker() {
	if (saveMaxTime_ == mainTimer_->maxTime_)return;
	saveMaxTime_ = mainTimer_->maxTime_;

	int markerNum10 = (int)saveMaxTime_ - 1;
	marker10_->objectParts_.clear();

	for (int i = 0; i < markerNum10; i++) {
		marker10_->CreateDefaultData();
		float posX = startPos_.x + ((EndPos_.x - startPos_.x) / mainTimer_->maxTime_) * (i + 1);
		float posY = startPos_.y;
		marker10_->objectParts_[i].materialConfig->textureHandle = markerTH_;
		marker10_->objectParts_[i].transform.translate = { posX, posY, 0.0f };
		marker10_->objectParts_[i].transform.scale = { 1.0f, 0.3f, 1.0f };
		marker10_->objectParts_[i].materialConfig->textureColor = markerColor_;
	}

	int markerNum05{};
	if (saveMaxTime_ <= 50)	markerNum05 = int(saveMaxTime_ * 5.0f);
	if (saveMaxTime_ > 50 && saveMaxTime_ <= 150) markerNum05 = int(saveMaxTime_ * 2.0f);
	marker02_->objectParts_.clear();

	if (markerNum05 != 0) {
		for (int i = 0; i < markerNum05; i++) {
			if ((saveMaxTime_ <= 50) && (i % 5 == 0)) {
				continue;
			}
			if ((saveMaxTime_ > 50 && saveMaxTime_ <= 150) && (i % 2 == 0)) {
				continue;
			}

			marker02_->CreateDefaultData();
			float posX = startPos_.x + ((EndPos_.x - startPos_.x) / markerNum05) * (i);
			float posY = startPos_.y;
			marker02_->objectParts_.back().materialConfig->textureHandle = markerTH_;
			marker02_->objectParts_.back().transform.translate = { posX, posY, 0.0f };
			marker02_->objectParts_.back().transform.scale = { 1.0f, 0.2f, 1.0f };
			marker02_->objectParts_.back().materialConfig->textureColor = markerColor2_;
		}
	}
}

void AnimationEditor::KeyFrameTurnning() {
	if (isChooseKeyFrame_) {
		if (system_->GetMouseTriggerOn(0)) {

			auto iterator = std::find_if(keyFrameList_.begin(), keyFrameList_.end(),
				[this](const KeyFrame& kf) {
					return kf.index_ == selectedKeyFrame_;
				});
			int index = int(iterator - keyFrameList_.begin());

			//SetChooseModel(&animationListKey_.transformData);
			SetChooseModel(&animationList_[0].keyList[index].transformData);
			//SetChooseModel(&animationList_[0].keyList[selectedKeyFrame_].transformData);
		}
	} else {
		keyFrameModel_ = nullptr;
	}
}


void AnimationEditor::CreateKeyFrame(float time_) {
	if (time_ == -1) {
		ping_->CreateDefaultData();
		ping_->objectParts_.back().materialConfig->textureHandle = pingTH_;
		ping_->objectParts_.back().transform.translate = {
			NowPos_.x ,
			(float)config::GetClientHeight() - 10.0f,
			100.0f
		};
		ping_->objectParts_.back().anchorPoint = { 5.0f,0.0f };

		KeyFrame newKeyFrame;
		newKeyFrame.index_ = (int)keyFrameList_.size();
		newKeyFrame.time_ = mainTimer_->parameter_;
		newKeyFrame.hitBox_ = {
			{ ping_->objectParts_.back().transform.translate.x ,
			  ping_->objectParts_.back().transform.translate.y + 5.0f },
			10.0f, 10.0f
		};
		keyFrameList_.push_back(newKeyFrame);

		animationList_[0].AddKeyFrame();

	} else {
		ping_->CreateDefaultData();
		ping_->objectParts_.back().materialConfig->textureHandle = pingTH_;
		float Leight = EndPos_.x - startPos_.x;
		float timeParameter = time_ / mainTimer_->maxTime_;
		ping_->objectParts_.back().transform.translate = {
			startPos_.x + Leight * timeParameter,
			(float)config::GetClientHeight() - 10.0f,
			100.0f
		};
		ping_->objectParts_.back().anchorPoint = { 5.0f,0.0f };
		KeyFrame newKeyFrame;
		newKeyFrame.index_ = (int)keyFrameList_.size();
		newKeyFrame.time_ = time_;
		newKeyFrame.hitBox_ = {
			{ ping_->objectParts_.back().transform.translate.x ,
			  ping_->objectParts_.back().transform.translate.y + 5.0f },
			10.0f, 10.0f
		};
		keyFrameList_.push_back(newKeyFrame);

		animationList_[0].AddKeyFrame(time_);
	}

}

void AnimationEditor::DeleteKeyFrame(KeyFrame* keyFrame) {
	if (keyFrame == nullptr) {
		auto keyFrameListTarget = std::find_if(keyFrameList_.begin(), keyFrameList_.end(),
			[&](const KeyFrame& kf) {
				return kf.index_ == selectedKeyFrame_;
			}
		);

		int index = int(keyFrameListTarget - keyFrameList_.begin());
		if (keyFrameListTarget != keyFrameList_.end())keyFrameList_.erase(keyFrameListTarget);

		for (int i = 0; i < keyFrameList_.size(); ++i) {
			keyFrameList_[i].index_ = i;
		}


		auto aniamtionListTarget = std::find_if(animationList_[0].keyList.begin(), animationList_[0].keyList.end(),
			[&](const KeyFrame& kf) {
				return kf.index_ == selectedKeyFrame_;
			}
		);
		if (aniamtionListTarget != animationList_[0].keyList.end())animationList_[0].keyList.erase(aniamtionListTarget);

		for (auto& objectData : animationList_) {
			for (int i = 0; i < objectData.keyList.size(); ++i) {
				objectData.keyList[i].index_ = i;
			}
		}

		if (!ping_->objectParts_.empty() && index < ping_->objectParts_.size()) {
			ping_->objectParts_.erase(ping_->objectParts_.begin() + index);
		}


		pickedKeyFrame_ = -1;
		selectedKeyFrame_ = -1;
		isPickKeyFrame_ = false;
		isChooseKeyFrame_ = false;
		keyFrameModel_ = nullptr;
		choosingModel_ = nullptr;

	} else {
		int targetIndex = keyFrame->index_;
		auto target = std::find_if(keyFrameList_.begin(), keyFrameList_.end(),
			[&](const KeyFrame& kf) {
				return kf.index_ == targetIndex;
			}
		);
	}
}

void AnimationEditor::SortKeyFrame() {

	std::sort(keyFrameList_.begin(), keyFrameList_.end(),
		[](const KeyFrame& a, const KeyFrame& b) {
			return a.time_ < b.time_;
		}
	);

	std::sort(ping_->objectParts_.begin(), ping_->objectParts_.end(),
		[](const SpritePart& a, const SpritePart& b) {
			return a.transform.translate.x < b.transform.translate.x;
		}
	);

	for (auto& objectData : animationList_) {

		for (int i = 0; i < objectData.keyList.size(); i++) {
			if (keyFrameList_[i].time_ != objectData.keyList[i].time_) {
				objectData.keyList[i].time_ = keyFrameList_[i].time_;
			}
		}

		for (int i = 0; i < keyFrameList_.size(); i++) {
			auto it = std::find_if(objectData.keyList.begin(), objectData.keyList.end(),
				[&](const KeyFrame& kf) { return kf.index_ == keyFrameList_[i].index_; });
			if (it != objectData.keyList.end()) {
				// 把它移動到正確位置
				std::iter_swap(objectData.keyList.begin() + i, it);
			}
		}

	}
}

void AnimationEditor::AdjuctKeyFrameTexture() {
	for (int i = 0; i < ping_->objectParts_.size(); i++) {
		if (i == 0) {
			ping_->objectParts_[i].materialConfig->textureHandle = pingFirstTH_;
			continue;
		}
		if (i == (int)ping_->objectParts_.size() - 1) {
			ping_->objectParts_[i].materialConfig->textureHandle = pingLastTH_;
			continue;
		}
		ping_->objectParts_[i].materialConfig->textureHandle = pingTH_;
	}
}

void AnimationEditor::SaveAnimationData(const AnimationObjectData& animationList, const std::string& filePath) {
	nlohmann::json file{};

	file["AnimationID"] = animationList.animationID;
	file["KeyFrameNum"] = animationList.keyList.size();
	file["ObjectPartNum"] = animationList.SimpleObject.objectParts_.size();
	for (const auto& kf : animationList.keyList) {
		nlohmann::json keyFrameJson;
		keyFrameJson["Index"] = kf.index_;
		keyFrameJson["Time"] = kf.time_;
		keyFrameJson["AnimationType"] = (int)kf.animationType_;
		keyFrameJson["EaseRate"] = kf.easeRate_;

		// MainPosition
		keyFrameJson["TransformData"]["MainPosition"] = {
			{"TranslateX", kf.transformData.mainPosition.transform.translate.x},
			{"TranslateY", kf.transformData.mainPosition.transform.translate.y},
			{"TranslateZ", kf.transformData.mainPosition.transform.translate.z},
			{"RotateX",    kf.transformData.mainPosition.transform.rotate.x},
			{"RotateY",    kf.transformData.mainPosition.transform.rotate.y},
			{"RotateZ",    kf.transformData.mainPosition.transform.rotate.z},
			{"ScaleX",     kf.transformData.mainPosition.transform.scale.x},
			{"ScaleY",     kf.transformData.mainPosition.transform.scale.y},
			{"ScaleZ",     kf.transformData.mainPosition.transform.scale.z}
		};

		// ObjectParts
		for (const auto& part : kf.transformData.objectParts_) {
			keyFrameJson["TransformData"]["ObjectPart"].push_back({
				{"TranslateX", part.transform.translate.x},
				{"TranslateY", part.transform.translate.y},
				{"TranslateZ", part.transform.translate.z},
				{"RotateX",    part.transform.rotate.x},
				{"RotateY",    part.transform.rotate.y},
				{"RotateZ",    part.transform.rotate.z},
				{"ScaleX",     part.transform.scale.x},
				{"ScaleY",     part.transform.scale.y},
				{"ScaleZ",     part.transform.scale.z}
				});
		}

		file["KeyFrameList"].push_back(keyFrameJson);
	}

	std::ofstream output(filePath);
	output << file.dump(4);
}

void AnimationEditor::LoadAnimationData(AnimationObjectData* animationList, const std::string& filePath) {
	std::ifstream input(filePath);
	if (!input.is_open()) {
		throw std::runtime_error("Failed to open file: " + filePath);
	}

	nlohmann::json file;
	input >> file;  // 讀取並解析 JSON

	// 基本資訊
	animationList->keyList.clear();
	animationList->animationID = file["AnimationID"].get<int>();

	// KeyFrameList
	for (const auto& kfJson : file["KeyFrameList"]) {
		KeyFrame kf;
		kf.index_ = kfJson["Index"].get<int>();
		kf.time_ = kfJson["Time"].get<float>();
		kf.animationType_ = static_cast<AnimationType>(kfJson["AnimationType"].get<int>());
		kf.easeRate_ = kfJson["EaseRate"].get<float>();

		// MainPosition
		auto mp = kfJson["TransformData"]["MainPosition"];
		kf.transformData.mainPosition.transform.translate.x = mp["TranslateX"].get<float>();
		kf.transformData.mainPosition.transform.translate.y = mp["TranslateY"].get<float>();
		kf.transformData.mainPosition.transform.translate.z = mp["TranslateZ"].get<float>();
		kf.transformData.mainPosition.transform.rotate.x = mp["RotateX"].get<float>();
		kf.transformData.mainPosition.transform.rotate.y = mp["RotateY"].get<float>();
		kf.transformData.mainPosition.transform.rotate.z = mp["RotateZ"].get<float>();
		kf.transformData.mainPosition.transform.scale.x = mp["ScaleX"].get<float>();
		kf.transformData.mainPosition.transform.scale.y = mp["ScaleY"].get<float>();
		kf.transformData.mainPosition.transform.scale.z = mp["ScaleZ"].get<float>();

		// ObjectParts
		for (const auto& partJson : kfJson["TransformData"]["ObjectPart"]) {
			ObjectPart part;
			part.transform.translate.x = partJson["TranslateX"].get<float>();
			part.transform.translate.y = partJson["TranslateY"].get<float>();
			part.transform.translate.z = partJson["TranslateZ"].get<float>();
			part.transform.rotate.x = partJson["RotateX"].get<float>();
			part.transform.rotate.y = partJson["RotateY"].get<float>();
			part.transform.rotate.z = partJson["RotateZ"].get<float>();
			part.transform.scale.x = partJson["ScaleX"].get<float>();
			part.transform.scale.y = partJson["ScaleY"].get<float>();
			part.transform.scale.z = partJson["ScaleZ"].get<float>();

			kf.transformData.objectParts_.push_back(part);
		}

		animationList->keyList.push_back(kf);
	}
	keyFrameList_.clear();
	for (auto& kf : animationList->keyList) {
		KeyFrame newKf;
		newKf.index_ = kf.index_;
		newKf.time_ = kf.time_;
		newKf.animationType_ = kf.animationType_;
		newKf.easeRate_ = kf.easeRate_;
		newKf.transformData = kf.transformData;
		newKf.hitBox_ = {
			{ startPos_.x + ((EndPos_.x - startPos_.x) / mainTimer_->maxTime_) * kf.time_,
			  (float)config::GetClientHeight() - 10.0f + 5.0f },
			10.0f, 10.0f
		};
		keyFrameList_.push_back(newKf);
	}

	ping_->objectParts_.clear();
	for (const auto& kf : keyFrameList_) {
		ping_->CreateDefaultData();
		float Leight = EndPos_.x - startPos_.x;
		float timeParameter = kf.time_ / mainTimer_->maxTime_;
		ping_->objectParts_.back().materialConfig->textureHandle = pingTH_;
		ping_->objectParts_.back().transform.translate = {
			startPos_.x + Leight * timeParameter,
			(float)config::GetClientHeight() - 10.0f,
			100.0f
		};
		ping_->objectParts_.back().anchorPoint = { 5.0f,0.0f };
	}
}

#ifdef USE_IMGUI
void AnimationEditor::ImguiPart() {

	ImGui::Begin("File");
	ImGui::InputText("SavePath", &saveFilePath_);
	if (ImGui::Button("Save")) {
		SaveAnimationData(animationList_[0], saveFilePath_);
	}
	ImGui::InputText("LoadPath", &loadFilePath_);
	if (ImGui::Button("Load")) {
		LoadAnimationData(&animationList_[0], loadFilePath_);
	}
	ImGui::End();


	ImGui::Begin("SetTime");
	ImGui::Text("Time Editor: %.02f, %.02f", mainTimer_->parameter_, mainTimer_->maxTime_);
	ImGui::InputFloat("MaxTime", &mainTimer_->maxTime_);
	if (mainTimer_->maxTime_ > 1000)mainTimer_->maxTime_ = 1000;
	if (ImGui::Button("Play")) {
		if (isPlay_) {
			isPlay_ = false;
		} else {
			isPlay_ = true;
		}
	}
	if (ImGui::Button("ResetTime")) {
		mainTimer_->Reset0();
	}
	ImGui::Checkbox("Loop", &isLoop_);
	ImGui::Checkbox("PickKeyFrame", &isPickKeyFrame_);
	ImGui::Checkbox("ChooseKeyFrame", &isChooseKeyFrame_);
	ImGui::NewLine();
	ImGui::ColorEdit4("Marker1 Color", &markerColor_.x);
	ImGui::ColorEdit4("Marker2 Color", &markerColor2_.x);
	ImGui::End();


	ImGui::Begin("AnimtionEditor");
	if (ImGui::CollapsingHeader("pingInformation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("AllKeyFrame Num: %d", (int)keyFrameList_.size());
		if (selectedKeyFrame_ != -1) {
			ImGui::Text("SelectKeyFrame: %d", selectedKeyFrame_);
			auto iterator = std::find_if(keyFrameList_.begin(), keyFrameList_.end(),
				[this](const KeyFrame& kf) {
					return kf.index_ == selectedKeyFrame_;
				});
			KeyFrame& listKeyFrame = *iterator;

			ImGui::SliderFloat("KeyFrameTime", &listKeyFrame.time_, 0, mainTimer_->maxTime_);

			auto iterator2 = std::find_if(animationList_[0].keyList.begin(), animationList_[0].keyList.end(),
				[this](const KeyFrame& kf) {
					return kf.index_ == selectedKeyFrame_;
				});
			KeyFrame& animationListKeyFrame = *iterator2;
			const char* animationType[] = { "LINEARITY", "EASY_IN","EASY_OUT","EASY_IN_OUT","EASY_IN_BACK","EASY_OUT_BACK" };
			int typeIndex = static_cast<int>(animationListKeyFrame.animationType_);
			if (ImGui::Combo("AnimationType", &typeIndex, animationType, IM_ARRAYSIZE(animationType))) {
				animationListKeyFrame.animationType_ = static_cast<AnimationType>(typeIndex);
			}
			if (animationListKeyFrame.animationType_ != AnimationType::LINEARITY) {
				ImGui::SliderFloat("EaseRate", &animationListKeyFrame.easeRate_, 0.0f, 5.0f);
			}
		} else {
			ImGui::Text("SelectKeyFrame: --");
			ImGui::Text("KeyFrame Time: --");
		}
		if (ImGui::Button("Add KeyFrame")) {
			CreateKeyFrame();
		}
		if (ImGui::Button("Delete KeyFrame")) {
			DeleteKeyFrame();
		}

		ImGui::Text("KeyFrame num: %d", keyFrameList_.size());
		ImGui::Text("ObjectPart num: %d", ping_->objectParts_.size());
		ImGui::Text("animationObjectDataList_ num: %d", animationList_.size());

	}
	if (keyFrameModel_ != nullptr) {
		if (ImGui::CollapsingHeader("Object", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Indent();
			if (ImGui::CollapsingHeader("Main", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat3("Position", &keyFrameModel_->mainPosition.transform.translate.x, -10.0f, 10.0f);
				ImGui::InputFloat3("Position_Input", &keyFrameModel_->mainPosition.transform.translate.x);

				ImGui::SliderFloat3("Rotate", &keyFrameModel_->mainPosition.transform.rotate.x, -10.0f, 10.0f);
				ImGui::InputFloat3("Rotate_Input", &keyFrameModel_->mainPosition.transform.rotate.x);

				ImGui::SliderFloat3("Scale", &keyFrameModel_->mainPosition.transform.scale.x, -10.0f, 10.0f);
				ImGui::InputFloat3("Scale_Input", &keyFrameModel_->mainPosition.transform.scale.x);

			

			

			}
			ImGui::Indent();
			for (int i = 0; i < keyFrameModel_->objectParts_.size(); i++) {
				std::string index = std::to_string(i);
				if (ImGui::CollapsingHeader((std::string("ObjectPart_") + index).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::SliderFloat3((std::string("Position_") + index).c_str(), &keyFrameModel_->objectParts_[i].transform.translate.x, -10.0f, 10.0f);
					ImGui::InputFloat3((std::string("Position_Input_") + index).c_str(),
						&keyFrameModel_->objectParts_[i].transform.translate.x);

					
					ImGui::SliderFloat3((std::string("Rotate_") + index).c_str(), &keyFrameModel_->objectParts_[i].transform.rotate.x, -10.0f, 10.0f);
					ImGui::InputFloat3((std::string("Rotate_Input_") + index).c_str(),
						&keyFrameModel_->objectParts_[i].transform.rotate.x);

					
					
					ImGui::SliderFloat3((std::string("Scale_") + index).c_str(), &keyFrameModel_->objectParts_[i].transform.scale.x, -10.0f, 10.0f);
					ImGui::InputFloat3((std::string("Scale_Input_") + index).c_str(),
						&keyFrameModel_->objectParts_[i].transform.scale.x);


			

				
				}
			}
			ImGui::Unindent();
			ImGui::Unindent();
		}
	}
	ImGui::End();

}
#endif






