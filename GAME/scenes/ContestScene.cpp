#include "ContestScene.h"

ContestScene::ContestScene(kEngine *system) {
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

ContestScene::~ContestScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);
  system_->RemoveLight(light1_);

  delete light1_;
}

void ContestScene::Update() {
  CameraPart();

  // スペースキーでお題発表シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::NEXT;
  }

  // タイトルへ戻る
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_1)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETURN;
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = nextOutcome_;
  }
}

void ContestScene::Draw() {
#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("ContestScene");
  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}

void ContestScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}