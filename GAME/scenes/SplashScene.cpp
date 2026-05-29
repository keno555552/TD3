#include "SplashScene.h"
#include <dinput.h>
#include "kEngine.h"

SplashScene::SplashScene(kEngine* system) {
	system_ = system;

	camera_ = system_->CreateCamera();
	system_->SetCamera(camera_);

	// エンジンのバグ（ライトが0個だとクラッシュする）を回避するためのダミーライト
	dummyLight_ = std::make_unique<Light>();
	dummyLight_->intensity = 0.0f; // 影響を与えないようにする
	system_->AddLight(dummyLight_.get());

	splashTextureHandle_ = system_->LoadTexture("GAME/resources/splash_controls.png");
	
	splashSprite_ = std::make_unique<SimpleSprite>();
	splashSprite_->IntObject(system_);
	splashSprite_->CreateDefaultData();
	
	if (splashTextureHandle_ != -1 && !splashSprite_->objectParts_.empty()) {
		splashSprite_->objectParts_[0].materialConfig->textureHandle = splashTextureHandle_;
		splashSprite_->objectParts_[0].anchorPoint = { 640.0f, 360.0f };
		splashSprite_->objectParts_[0].cropSize = { 1280.0f, 720.0f };
	}

	splashSprite_->mainPosition.transform.translate = { 640.0f, 360.0f, 0.0f };
	
	splashSprite_->Update(camera_.lock().get());

	fade_.Initialize(system_);
	fade_.StartFadeIn(0.02f);
}

SplashScene::~SplashScene() {
	if (camera_.expired()) {
		system_->DestroyCamera(camera_);
	}
	if (dummyLight_) {
		system_->RemoveLight(dummyLight_.get());
	}
}

void SplashScene::Update() {
	if (splashSprite_) {
		splashSprite_->Update(camera_.lock().get());
	}
	fade_.Update(camera_.lock().get());

	if (!isFadingOut_) {
		float dt = system_->GetDeltaTime();
		if (dt > 0.1f) dt = 0.016f; // ロード時の巨大なdtを無視
		displayTimer_ += dt;

		bool canSkip = (displayTimer_ > 0.5f); // 最初の0.5秒はスキップ無効

		if (displayTimer_ >= kDisplayDuration_ ||
			(canSkip && (system_->GetTriggerOn(DIK_SPACE) ||
			system_->GetTriggerOn(DIK_RETURN) ||
			system_->GetMouseTriggerOn(0)))) {
			
			isFadingOut_ = true;
			fade_.StartFadeOut(0.02f);
		}
	} else {
		if (fade_.IsFinished()) {
			outcome_ = SceneOutcome::NEXT;
		}
	}
}

void SplashScene::Draw() {
	if (splashSprite_) {
		splashSprite_->Draw();
	}
	fade_.Draw();
}
