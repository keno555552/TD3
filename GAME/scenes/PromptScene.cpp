#include "PromptScene.h"
#include "GAME/actor/prompt/PromptBoard.h"
#include "GAME/actor/prompt/PromptData.h"
#include "Object/Object.h"
#include "config.h"
#include <time.h>
#include "Scenes/SceneManager.h"

PromptScene::PromptScene(kEngine *system) {
  system_ = system;

  PromptData::Clear();

  srand((unsigned int)time(NULL));

  // 全体を暗くするライト（方向ライト）
  light1_ = std::make_unique<Light>();
  light1_->direction = {-0.0f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 0.0f; // 全体を暗くする
  system_->AddLight(light1_.get());

  // スポットライトの追加
  spotLight_ = std::make_unique<Light>();
  spotLight_->lightingType = LightingType::SpotLight;
  spotLight_->position = {0.0f, 0.0f, 30.0f};
  spotLight_->direction = {0.0f, -0.1f, 1.0f};
  spotLight_->angle = 3.14159f / 4.0f;
  spotLight_->range = 100.0f;
  spotLight_->intensity = 18.5f;
  spotLight_->color = {1.0f, 1.0f, 1.0f};
  system_->AddLight(spotLight_.get());

  // 背景の壁（ライトを受けるため）
  int backScreenModel =
      system_->SetModelObj("GAME/resources/ContestStageObject/backScreen.obj");
  backgroundWall_ = new Object;
  backgroundWall_->IntObject(system_);
  backgroundWall_->CreateModelData(backScreenModel);
  backgroundWall_->mainPosition.transform = CreateDefaultTransform();
  backgroundWall_->mainPosition.transform.translate = {-5.0f, -50.0f, 130.0f};
  backgroundWall_->mainPosition.transform.rotate = {0.0f, 3.14159f, 0.0f};
  backgroundWall_->mainPosition.transform.scale = {20.0f, 20.0f, 20.0f};

  // カメラ
  debugCamera_ = system_->CreateDebugCamera();
  debugCamera_.lock()->SetFarClip(200.0f);
  camera_ = system_->CreateCamera();
  camera_.lock()->SetFarClip(200.0f);
  usingCamera_ = camera_;

  // 上部見出しUI画像の追加
  int titleTex =
      system_->LoadTexture("GAME/resources/texture/currentTheme.png");
  titleSprite_ = new SimpleSprite();
  titleSprite_->IntObject(system_);
  titleSprite_->CreateDefaultData();
  titleSprite_->objectParts_[0].materialConfig->textureHandle = titleTex;
  titleSprite_->objectParts_[0].anchorPoint = {256.0f, 64.0f};
  titleSprite_->mainPosition.transform.translate = {640.0f, 150.0f, 0.0f};
  system_->SetCamera(usingCamera_.lock());

  // お題マネージャー生成・全お題読み込み
  themeManager_ = new ThemeManager("GAME/resources/themes/");

  // 審査員マネージャー生成・全審査員読み込み
  judgeManager_ = new JudgeManager("GAME/resources/judges/");

  // お題演出ボード
  promptBoard_ = std::make_unique<PromptBoard>();
  promptBoard_->Initialize(
      system_, {static_cast<float>(config::GetClientWidth()) * 0.5f - 250.0f,
                static_cast<float>(config::GetClientHeight()) * 0.5f - 75.0f});

  if (themeManager_) {
    std::vector<int> textures;
    for (const auto &theme : themeManager_->GetAllThemes()) {
      int handle = system_->LoadTexture(theme.texturePath);
      textures.push_back(handle);
    }
    promptBoard_->SetThemeTextures(textures);
  }

  rollState_ = PromptRollState::Rolling;
  stopInputLockCounter_ = 0;
  selectedPromptShowCounter_ = 0;

  themeButton_ = std::make_unique<DetailButton>(system);
  themeButton_->SetButton({640.0f, 650.0f}, 400.0f, 80.0f);

  font_.Initialize(system_);

  Logger::Log("[PromptScene] font address=%p", &font_);

  // ドラムロールのロードと再生開始（wavファイルでBGM枠でループ再生）
  drumrollSoundHandle_ =
      system_->SoundLoadSE("GAME/resources/sounds/drumroll.wav");
  if (drumrollSoundHandle_ != -1) {
    system_->SoundPlayBGM(drumrollSoundHandle_, 1.0f);
  }

  // ドラムロールの締め（決定時）の音のロード
  drumrollEndSoundHandle_ =
      system_->SoundLoadSE("GAME/resources/sounds/drumroll_end.mp3");

  // フェード
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

PromptScene::~PromptScene() {
  if (drumrollSoundHandle_ != -1) {
    system_->SoundStop(drumrollSoundHandle_);
  }
  if (drumrollEndSoundHandle_ != -1) {
    system_->SoundStop(drumrollEndSoundHandle_);
  }

  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);
  system_->RemoveLight(light1_.get());

  system_->RemoveLight(spotLight_.get());
  spotLight_.reset();

  delete backgroundWall_;

  delete titleSprite_;

  font_.Cleanup();
  light1_.reset();

  delete themeManager_;
  themeManager_ = nullptr;

  delete judgeManager_;
  judgeManager_ = nullptr;

  // クリーンアップ：不要になったマテリアル（GPUバッファ）を解放する
  ResourceManager::GetInstance()->CleanupUnusedMaterials();
}

void PromptScene::Update() {
  themeButton_->Update();

  CameraPart();

  if (!fade_.IsBusy()) {
    UpdatePromptRoll();
  }

  if (promptBoard_ != nullptr) {
    promptBoard_->Update(usingCamera_.lock().get());
  }

  // スポットライトの演出更新
  if (rollState_ == PromptRollState::Rolling) {
    // パタパタ中はライトの位置と向きを振って探るような動き（8の字・無限大を描く）
    spotLightTimer_ += 0.03f;
    spotLight_->position.x = std::sin(spotLightTimer_) * 15.0f;
    spotLight_->position.y = 5.0f + std::sin(spotLightTimer_ * 2.0f) * 3.0f;

    spotLight_->direction.x = std::sin(spotLightTimer_) * 0.15f;
    spotLight_->direction.y = -0.1f + std::sin(spotLightTimer_ * 2.0f) * 0.05f;

    spotLight_->intensity = 5.5f; // 通常の明るさ
    spotLight_->color = {1.0f, 1.0f, 1.0f};
  } else if (rollState_ == PromptRollState::Stopped) {
    spotLight_->position.x += (0.0f - spotLight_->position.x) * 0.15f;
    spotLight_->position.y += (0.0f - spotLight_->position.y) * 0.15f;
    spotLight_->direction.x += (0.0f - spotLight_->direction.x) * 0.15f;
    spotLight_->direction.y += (0.0f - spotLight_->direction.y) * 0.15f;

    // お題決定時、演出が終わったらバシッと真ん中を強く照らす
    if (promptBoard_ != nullptr && promptBoard_->IsStopAnimationFinished()) {
      spotLight_->intensity = 11.0f;
      spotLight_->color = {1.0f, 0.98f, 0.9f};
    }
  }

  if (backgroundWall_ != nullptr) {
    backgroundWall_->Update(usingCamera_.lock().get());
  }

  if (titleSprite_ != nullptr) {
    titleSprite_->Update(usingCamera_.lock().get());
  }

  // フェード更新
  fade_.Update(usingCamera_.lock().get());

  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = SceneOutcome::NEXT;
  }
}

void PromptScene::UpdatePromptRoll() {
  switch (rollState_) {
  case PromptRollState::Rolling:
    ++stopInputLockCounter_;

    // 入力ロック解除後にスペースキーでお題決定
    if (stopInputLockCounter_ >= stopInputLockFrame_) {
      if (system_->GetTriggerOn(DIK_SPACE) || themeButton_->GetIsClicked()) {
        DecidePrompt();
      }
    }
    break;

  case PromptRollState::Stopped:
    if (promptBoard_ != nullptr && promptBoard_->IsStopAnimationFinished()) {

      // アニメーションが完了し、パネルがバーンと大きくなった瞬間に音を切り替える
      if (!isEndSoundPlayed_) {
        if (drumrollSoundHandle_ != -1) {
          system_->SoundStop(drumrollSoundHandle_);
        }
        if (drumrollEndSoundHandle_ != -1) {
          system_->SoundPlaySE(drumrollEndSoundHandle_, 1.0f);
        }
        isEndSoundPlayed_ = true;
      }

      // スペースキーで次のシーンへ
      if (system_->GetTriggerOn(DIK_SPACE) || themeButton_->GetIsClicked()) {
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
  // ThemeManager からランダムにお題を選出
  selectedTheme_ = themeManager_->SelectRandom();

  if (selectedTheme_ == nullptr) {
    return;
  }

  // 審査員を 3 人選出して PromptData に保存
  auto selectedJudges = judgeManager_->SelectRandom(3);
  std::vector<JudgeData> judgesCopy;
  for (const auto *judge : selectedJudges) {
    if (judge != nullptr) {
      judgesCopy.push_back(*judge);
    }
  }
  PromptData::SetJudges(judgesCopy);

  // PromptData にお題名とテクスチャパスと ThemeData を保存
  PromptData::SetSelectedPrompt(selectedTheme_->themeName,
                                selectedTheme_->texturePath);
  PromptData::SetThemeData(*selectedTheme_);

  // 選ばれたお題のテクスチャをボードにセット
  if (promptBoard_ != nullptr) {
    int textureHandle = system_->LoadTexture(selectedTheme_->texturePath);
    promptBoard_->SetPromptTexture(textureHandle);
    promptBoard_->StartStopAnimation();
  }

  selectedPromptShowCounter_ = 0;
  rollState_ = PromptRollState::Stopped;
}

void PromptScene::Draw() {
  bool currentPause = SceneManager::GetInstance().IsPause();
  if (currentPause && !wasPaused_) {
    if (drumrollSoundHandle_ != -1) {
      system_->SoundPause(drumrollSoundHandle_);
    }
    if (drumrollEndSoundHandle_ != -1) {
      system_->SoundPause(drumrollEndSoundHandle_);
    }
  } else if (!currentPause && wasPaused_) {
    if (drumrollSoundHandle_ != -1) {
      system_->SoundContinue(drumrollSoundHandle_);
    }
    if (drumrollEndSoundHandle_ != -1) {
      system_->SoundContinue(drumrollEndSoundHandle_);
    }
  }
  wasPaused_ = currentPause;

  if (backgroundWall_ != nullptr) {
    backgroundWall_->Draw();
  }

  if (promptBoard_ != nullptr) {
    promptBoard_->Draw();
  }

  if (titleSprite_ != nullptr) {
    titleSprite_->Draw();
  }

  themeButton_->Render();

  if (promptBoard_ != nullptr && promptBoard_->IsStopAnimationFinished()) {
    font_.RenderText("改造スタート！", {640.0f, 620.0f}, 48.0f,
                     BitmapFont::Align::Center, 5, {1.0f, 1.0f, 0.0f, 1.0f});
  } else {
    font_.RenderText("お題発表！", {640.0f, 620.0f}, 48.0f,
                     BitmapFont::Align::Center, 5, {1.0f, 1.0f, 0.0f, 1.0f});
  }

#ifdef USE_IMGUI
  ImGui::Begin("Scene");
  ImGui::Text("PromptScene");

  if (selectedTheme_ != nullptr) {
    ImGui::Text("Theme: %s", selectedTheme_->themeName.c_str());
    ImGui::Text("ID: %s", selectedTheme_->themeId.c_str());
    ImGui::Text("Category: %s", selectedTheme_->category.c_str());
    ImGui::Text("TexturePath: %s", selectedTheme_->texturePath.c_str());
  } else {
    ImGui::Text("Press SPACE to decide theme");
  }

  ImGui::Text("Loaded themes: %d",
              static_cast<int>(themeManager_->GetThemeCount()));

  ImGui::Text("Loaded judges: %d",
              static_cast<int>(judgeManager_->GetJudgeCount()));

  const auto *judges = PromptData::GetJudges();
  if (judges != nullptr) {
    ImGui::Separator();
    ImGui::Text("Selected Judges:");
    for (const auto &judge : *judges) {
      ImGui::Text("  %s - %s", judge.judgeId.c_str(), judge.judgeTitle.c_str());
    }
  }

  ImGui::End();
#endif

  fade_.Draw();
}

void PromptScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_.lock()->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_.lock());
}