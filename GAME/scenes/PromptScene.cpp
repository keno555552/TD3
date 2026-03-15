#include "PromptScene.h"

PromptScene::PromptScene(kEngine *system) {
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

  // お題マネージャー生成・全お題読み込み
  themeManager_ = new ThemeManager("GAME/resources/themes/");

  // フェード
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

PromptScene::~PromptScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);
  system_->RemoveLight(light1_);

  delete light1_;
  delete themeManager_;
  themeManager_ = nullptr;
}

void PromptScene::Update() {
  CameraPart();

  // スペースキーで改造シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  // T キーでお題選出
  if (system_->GetTriggerOn(DIK_T)) {
      selectedTheme_ = themeManager_->SelectRandom();
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = SceneOutcome::NEXT;
  }
}

void PromptScene::Draw() {
#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("PromptScene");
  if (selectedTheme_ != nullptr) {
      ImGui::Text("Theme: %s", selectedTheme_->themeName.c_str());
      ImGui::Text("ID: %s", selectedTheme_->themeId.c_str());
      ImGui::Text("Category: %s", selectedTheme_->category.c_str());
  } else {
      ImGui::Text("Theme: not selected (press T)");
  }
  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}

void PromptScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}