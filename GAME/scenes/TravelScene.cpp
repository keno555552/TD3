#include "TravelScene.h"

TravelScene::TravelScene(kEngine *system) {
  system_ = system;

  // ライト
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  // カメラ
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();
  usingCamera_ = camera_;
  system_->SetCamera(usingCamera_);

  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

TravelScene::~TravelScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  delete light1_;
}

void TravelScene::Update() {
  CameraPart();

  // スペースキーで評価シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::NEXT;
  }

  // リトライ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_RETURN)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETRY;
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = nextOutcome_;
  }
}

void TravelScene::Draw() {
#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("TravelScene");
  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}

void TravelScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}