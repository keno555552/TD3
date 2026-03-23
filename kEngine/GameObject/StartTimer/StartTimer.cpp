#include "StartTimer.h"

StartTimer::StartTimer(kEngine* kEngine) {
	SimpleSprite::IntObject(kEngine);
	timer_ = std::make_unique<Timer>();
	timer_->Init0(1.0f, kEngine->GetTimeManager());
	timer_->SetInfluenceByTimeScale(true);

	/// ========= テキスチャーハンドル =========///

	std::string basePath = "resources/texture/StartTimer/";

	numberTextureHandles_[2] = kEngine->LoadTexture(basePath + "3.png");
	numberTextureHandles_[1] = kEngine->LoadTexture(basePath + "2.png");
	numberTextureHandles_[0] = kEngine->LoadTexture(basePath + "1.png");
	startTextureHandle_ = kEngine->LoadTexture(basePath + "Start.png");

	/// ============ オブジェクト =============///

	for (int i = 0; i < 3; i++) {
		numberSprites_[i] = std::make_unique<SimpleSprite>();
		numberSprites_[i]->IntObject(kEngine);
		numberSprites_[i]->CreateDefaultData();
		numberSprites_[i]->mainPosition.transform.scale = { startScale_,startScale_,1.0f };
		numberSprites_[i]->mainPosition.transform.translate = { 640.0f,360.0f,0.0f };
		numberSprites_[i]->objectParts_[0].materialConfig->textureHandle = numberTextureHandles_[i];
		numberSprites_[i]->objectParts_[0].materialConfig->textureColor = usingColor;
		numberSprites_[i]->objectParts_[0].anchorPoint = { 422.0f,178.0f };
	}
	startSprite_ = std::make_unique<SimpleSprite>();
	startSprite_->IntObject(kEngine);
	startSprite_->CreateDefaultData();
	startSprite_->mainPosition.transform.scale = { startScale_,startScale_,1.0f };
	startSprite_->mainPosition.transform.translate = { 640.0f,360.0f,0.0f };
	startSprite_->objectParts_[0].materialConfig->textureHandle = startTextureHandle_;
	startSprite_->objectParts_[0].materialConfig->textureColor = usingColor;
	startSprite_->objectParts_[0].anchorPoint = { 422.0f,178.0f };
}

StartTimer::~StartTimer() {

	numberSprites_[0].reset();
	numberSprites_[1].reset();
	numberSprites_[2].reset();

	startSprite_.reset();

	timer_.reset();
}

void StartTimer::Update(Camera* camera) {

	if (!isStart_)return;

	/// ========= タイマー更新 ========= ///
	if (numberCount_ >= 0) {
		if (timer_->parameter_ != timer_->maxTime_) {
			timer_->ToMix();
		} else {
			numberCount_--;
			timer_->Reset0();
		}
	}

	/// ========= オブジェクト更新 ========= ///

	int movingObjectIndex = numberCount_ - 1;

	if (movingObjectIndex >= 0) {
		numberSprites_[movingObjectIndex]->objectParts_[0].materialConfig->textureColor.w = timer_->linearity(1.0f, 0.0f);
		float scaleValue = timer_->linearity(startScale_, 1.0f);
		numberSprites_[movingObjectIndex]->mainPosition.transform.scale = { scaleValue,scaleValue,1.0f };
	} else {
		startSprite_->objectParts_[0].materialConfig->textureColor.w = timer_->linearity(1.0f, 0.0f);
		float scaleValue = timer_->linearity(startScale_, 1.0f);
		startSprite_->mainPosition.transform.scale = { scaleValue,scaleValue,1.0f };
	}

	/// ========== スプライト配置 ========== ///

	

}

void StartTimer::Draw() {

	//ImGuiPart();
	if (numberCount_ < 0 && timer_->parameter_ == timer_->maxTime_)return;

	int movingObjectIndex = numberCount_ - 1;

	if (movingObjectIndex >= 0) {
		numberSprites_[movingObjectIndex]->Draw();
	} else if(movingObjectIndex == -1){
		startSprite_->Draw();
	}

}

void StartTimer::Start() {
	isStart_ = true;
}

void StartTimer::Stop() {
	isStart_ = false;
}

void StartTimer::Reset() {
	for (int i = 0; i < 3; i++) {
		numberSprites_[i]->mainPosition.transform.scale = { startScale_, startScale_, 1.0f };
		numberSprites_[i]->objectParts_[0].materialConfig->textureColor = usingColor;
	}
	startSprite_->mainPosition.transform.scale = { startScale_, startScale_, 1.0f };
	startSprite_->objectParts_[0].materialConfig->textureColor = usingColor;


	timer_->Reset0();
	numberCount_ = 3;
	isStart_ = false;


}

bool StartTimer::IsTimeUp() {
	if (numberCount_ > -1)return false;
	if (timer_->parameter_ != timer_->maxTime_)return false;
	return true;
}

Timer& StartTimer::GetTime() {
	return *timer_;
}

void StartTimer::ImGuiPart() {
#ifdef USE_IMGUI

	float scale = numberSprites_[0]->mainPosition.transform.scale.x;

	ImGui::Begin("Scale");
	ImGui::SliderFloat2("position", &numberSprites_[0]->mainPosition.transform.translate.x, 0.0f, 1280.0f);
	ImGui::SliderFloat("scale", &scale,0.1f,1.0f);
	ImGui::End();

	numberSprites_[0]->mainPosition.transform.scale = { scale ,scale ,1.0f };
#endif // DEBUG
}
