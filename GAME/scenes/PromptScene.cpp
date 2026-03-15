#include "PromptScene.h"
#include "GAME/actor/prompt/PromptBoard.h"
#include "GAME/actor/prompt/PromptData.h"
#include "config.h"

PromptScene::PromptScene(kEngine *system) {
  system_ = system;

  PromptData::Clear();

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
  InitializePrompts();

  promptBoard_ = std::make_unique<PromptBoard>();
  promptBoard_->Initialize(
      system_,
      {static_cast<float>(config::GetClientWidth()) * 0.5f - 250.0f,
       static_cast<float>(config::GetClientHeight()) * 0.5f - 75.0f});

  currentPromptIndex_ = 0;
  selectedPromptIndex_ = -1;
  rollState_ = PromptRollState::Rolling;
  rollFrameCounter_ = 0;
  stopInputLockCounter_ = 0;
  selectedPromptShowCounter_ = 0;

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

void PromptScene::InitializePrompts() {
  prompts_.clear();
  promptTexturePaths_.clear();
  promptTextureHandles_.clear();

  const std::string defaultPromptTexturePath =
      "GAME/resources/texture/prompt.png";

  prompts_.push_back("馬に好かれそうな見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("速そうに見える見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("威圧感のある見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("かわいさ全振りの見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("会場で目立ちそうな見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("強そうだけど不安定な見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("優雅そうな見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  prompts_.push_back("バランスが悪そうな見た目");
  promptTexturePaths_.push_back(defaultPromptTexturePath);

  for (const std::string &path : promptTexturePaths_) {
    promptTextureHandles_.push_back(system_->LoadTexture(path));
  }
}

void PromptScene::Update() {
  CameraPart();

  if (!fade_.IsBusy()) {
    UpdatePromptRoll();
  }

  if (promptBoard_ != nullptr) {
    promptBoard_->Update();
  }

  // T キーでお題選出
  if (system_->GetTriggerOn(DIK_T)) {
      selectedTheme_ = themeManager_->SelectRandom();
  }

  // フェード更新
  fade_.Update(usingCamera_);

  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = SceneOutcome::NEXT;
  }
}

void PromptScene::UpdatePromptRoll() {
  switch (rollState_) {
  case PromptRollState::Rolling:
    ++rollFrameCounter_;
    ++stopInputLockCounter_;

    if (rollFrameCounter_ >= rollIntervalFrame_) {
      rollFrameCounter_ = 0;
      currentPromptIndex_ =
          (currentPromptIndex_ + 1) % static_cast<int>(prompts_.size());
    }

    if (stopInputLockCounter_ >= stopInputLockFrame_) {
      if (system_->GetTriggerOn(DIK_SPACE)) {
        DecidePrompt();
      }
    }
    break;

  case PromptRollState::Stopped:
    if (promptBoard_ != nullptr && promptBoard_->IsStopAnimationFinished()) {
      ++selectedPromptShowCounter_;

      if (selectedPromptShowCounter_ >= selectedPromptShowFrame_) {
        fade_.StartFadeOut();
        isStartTransition_ = true;
        rollState_ = PromptRollState::FadeOut;
      }
    }
    break;

  case PromptRollState::FadeOut:
    break;
  }
}

void PromptScene::DecidePrompt() {
  selectedPromptIndex_ = currentPromptIndex_;

  PromptData::SetSelectedPrompt(prompts_[selectedPromptIndex_],
                                promptTexturePaths_[selectedPromptIndex_]);

  if (promptBoard_ != nullptr) {
    promptBoard_->SetPromptTexture(promptTextureHandles_[selectedPromptIndex_]);
    promptBoard_->StartStopAnimation();
  }

  selectedPromptShowCounter_ = 0;
  rollState_ = PromptRollState::Stopped;
}

const std::string &PromptScene::GetCurrentPrompt() const {
  return prompts_[currentPromptIndex_];
}

void PromptScene::Draw() {
  if (promptBoard_ != nullptr) {
    promptBoard_->Draw();
  }

#ifdef USE_IMGUI
  ImGui::Begin("Scene");
  ImGui::Text("PromptScene");
  if (selectedTheme_ != nullptr) {
      ImGui::Text("Theme: %s", selectedTheme_->themeName.c_str());
      ImGui::Text("ID: %s", selectedTheme_->themeId.c_str());
      ImGui::Text("Category: %s", selectedTheme_->category.c_str());
  } else {
      ImGui::Text("Theme: not selected (press T)");
  }
  ImGui::Separator();
  ImGui::Text("Press SPACE to stop");
  ImGui::Text("Current Prompt:");
  ImGui::Text("%s", GetCurrentPrompt().c_str());

  if (selectedPromptIndex_ >= 0) {
    ImGui::Text("Selected Prompt:");
    ImGui::Text("%s", prompts_[selectedPromptIndex_].c_str());
  }

  ImGui::End();
#endif

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