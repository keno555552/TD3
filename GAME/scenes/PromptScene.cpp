#include "PromptScene.h"

PromptScene::PromptScene(kEngine *system) {
  system_ = system;

  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();
  usingCamera_ = camera_;
  system_->SetCamera(usingCamera_);
}

PromptScene::~PromptScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  delete light1_;
}

void PromptScene::Update() {
  CameraPart();

  if (system_->GetTriggerOn(DIK_SPACE)) {
    outcome_ = SceneOutcome::NEXT;
  }
}

void PromptScene::Draw() {
#ifdef USE_IMGUI
  ImGui::Begin("Scene");
  ImGui::Text("PromptScene");
  ImGui::End();
#endif
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