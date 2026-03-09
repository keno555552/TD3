#include "TitleScene.h"

TitleScene::TitleScene(kEngine *system) {
  system_ = system;

  // 最低限のライト
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  // 最低限のカメラ
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();
  usingCamera_ = camera_;
  system_->SetCamera(usingCamera_);

  // フェード
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

TitleScene::~TitleScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);
  system_->RemoveLight(light1_);

  delete light1_;
}

void TitleScene::Update() {
  system_->SetCamera(usingCamera_);

  // スペースキーでお題発表シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = SceneOutcome::NEXT;
  }
}

void TitleScene::Draw() {
#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("TitleScene");
  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}