#include "PromptScene.h"
#include "GAME/actor/prompt/PromptBoard.h"
#include "GAME/actor/prompt/PromptData.h"
#include "config.h"

PromptScene::PromptScene(kEngine* system) {
    system_ = system;

    PromptData::Clear();

    // ライト
    light1_ = new Light;
    light1_->direction = { -0.5f, -1.0f, -0.3f };
    light1_->color = { 1.0f, 1.0f, 1.0f };
    light1_->intensity = 1.0f;
    system_->AddLight(light1_);

    // カメラ
    debugCamera_ = system_->CreateDebugCamera();
    camera_ = system_->CreateCamera();
    usingCamera_ = camera_;
    system_->SetCamera(usingCamera_);

    // お題マネージャー生成・全お題読み込み
    themeManager_ = new ThemeManager("GAME/resources/themes/");

    // お題演出ボード
    promptBoard_ = std::make_unique<PromptBoard>();
    promptBoard_->Initialize(
        system_,
        { static_cast<float>(config::GetClientWidth()) * 0.5f - 250.0f,
         static_cast<float>(config::GetClientHeight()) * 0.5f - 75.0f });

    rollState_ = PromptRollState::Rolling;
    stopInputLockCounter_ = 0;
    selectedPromptShowCounter_ = 0;

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

    if (!fade_.IsBusy()) {
        UpdatePromptRoll();
    }

    if (promptBoard_ != nullptr) {
        promptBoard_->Update();
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
        ++stopInputLockCounter_;

        // 入力ロック解除後にスペースキーでお題決定
        if (stopInputLockCounter_ >= stopInputLockFrame_) {
            if (system_->GetTriggerOn(DIK_SPACE)) {
                DecidePrompt();
            }
        }
        break;

    case PromptRollState::Stopped:
        if (promptBoard_ != nullptr && promptBoard_->IsStopAnimationFinished()) {
            // スペースキーで次のシーンへ
            if (system_->GetTriggerOn(DIK_SPACE)) {
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
        ImGui::Text("Press SPACE to decide theme");
    }

    ImGui::Text("Loaded themes: %d",
        static_cast<int>(themeManager_->GetThemeCount()));
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