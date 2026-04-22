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

  titleTextObject_ = new Object;

  titleTextModelHandle_ = system_->SetModelObj("GAME/resources/TitleScene/TitleText.obj");
  titleTextObject_->IntObject(system_);
  titleTextObject_->CreateModelData(titleTextModelHandle_);
  titleTextObject_->mainPosition.transform = CreateDefaultTransform();
  titleTextObject_->mainPosition.transform.translate = { 0.0f, 0.0f, 0.0f };
  titleTextObject_->mainPosition.transform.rotate = { 3.1415f/2.0f ,0.0f, 0.0f };
  titleTextObject_->mainPosition.transform.scale = { 1.0f,1.0f,1.0f };
}

TitleScene::~TitleScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);
  system_->RemoveLight(light1_);
  delete titleTextObject_;
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

    if (titleTextObject_) {
        titleTextObject_->Update(usingCamera_);
        titleTextObject_->Draw();
    }

#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("TitleScene");
  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}