#include "TrophyPart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"

TrophyPart::TrophyPart(kEngine* system, BitmapFont* font, Vector3 playerTarget)
	: IContestPart(system, font) 
{
	cameraTransform_ = { { playerTarget.x, playerTarget.y, playerTarget.z - 1.5f }, { 0.0f, 0.0f, 0.0f } };

	nextThemeButton_ = std::make_unique<DetailButton>(system);
	nextThemeButton_->SetButton({ 640.0f, 180.0f }, 400.0f, 80.0f);

	sameThemeButton_ = std::make_unique<DetailButton>(system);
	sameThemeButton_->SetButton({ 640.0f, 360.0f }, 400.0f, 80.0f);

	titleButton_ = std::make_unique<DetailButton>(system);
	titleButton_->SetButton({ 640.0f, 540.0f }, 400.0f, 80.0f);

	decideSoundHandle_ = system_->SoundLoadSE("GAME/resources/sounds/decide.mp3");
	selectSoundHandle_ = system_->SoundLoadSE("GAME/resources/sounds/select.mp3");
}

TrophyPart::~TrophyPart() {
}

void TrophyPart::Update() {
	if (choice_ != TrophyChoice::None) return;

	nextThemeButton_->Update();
	sameThemeButton_->Update();
	titleButton_->Update();

	float dt = system_->GetDeltaTime();
        if (menuInputCooldown_ > 0.0f) {
          menuInputCooldown_ -= dt;
          if (menuInputCooldown_ < 0.0f)
            menuInputCooldown_ = 0.0f;
        }

        Vector2 mouse = system_->GetMousePosVector2();
        bool isMouseMoved =
            (mouse.x != prevMousePos_.x || mouse.y != prevMousePos_.y);
        prevMousePos_ = mouse;

        if (isMouseMoved) {
          if (nextThemeButton_->GetIsSelect(mouse, 1, 1)) {
            menuSelection_ = TrophyChoice::NextTheme;
          } else if (sameThemeButton_->GetIsSelect(mouse, 1, 1)) {
            menuSelection_ = TrophyChoice::Retry;
          } else if (titleButton_->GetIsSelect(mouse, 1, 1)) {
            menuSelection_ = TrophyChoice::Title;
          }
        }

        if (menuInputCooldown_ <= 0.0f) {
          if (system_->GetTriggerOn(DIK_UP) || system_->GetTriggerOn(DIK_W)) {
            if (selectSoundHandle_ != -1) {
              system_->SoundPlaySE(selectSoundHandle_, 0.5f);
            }
            if (menuSelection_ == TrophyChoice::Retry) {
              menuSelection_ = TrophyChoice::NextTheme;
            } else if (menuSelection_ == TrophyChoice::Title) {
              menuSelection_ = TrophyChoice::Retry;
            } else if (menuSelection_ == TrophyChoice::NextTheme) {
              menuSelection_ = TrophyChoice::Title; // ループする
            }
            menuInputCooldown_ = 0.12f;
          }
          if (system_->GetTriggerOn(DIK_DOWN) || system_->GetTriggerOn(DIK_S)) {
            if (selectSoundHandle_ != -1) {
              system_->SoundPlaySE(selectSoundHandle_, 0.5f);
            }
            if (menuSelection_ == TrophyChoice::NextTheme) {
              menuSelection_ = TrophyChoice::Retry;
            } else if (menuSelection_ == TrophyChoice::Retry) {
              menuSelection_ = TrophyChoice::Title;
            } else if (menuSelection_ == TrophyChoice::Title) {
              menuSelection_ = TrophyChoice::NextTheme; // ループする
            }
            menuInputCooldown_ = 0.12f;
          }
        }

        nextThemeButton_->ForceSelectState(menuSelection_ ==
                                           TrophyChoice::NextTheme);
        sameThemeButton_->ForceSelectState(menuSelection_ ==
                                           TrophyChoice::Retry);
        titleButton_->ForceSelectState(menuSelection_ == TrophyChoice::Title);

        bool decide = system_->GetTriggerOn(DIK_SPACE) ||
                      system_->GetTriggerOn(DIK_RETURN);

        if (nextThemeButton_->GetIsRelease() ||
            (decide && menuSelection_ == TrophyChoice::NextTheme)) {
          if (decideSoundHandle_ != -1) system_->SoundPlaySE(decideSoundHandle_, 0.5f);
          choice_ = TrophyChoice::NextTheme;
        } else if (sameThemeButton_->GetIsRelease() ||
                   system_->GetTriggerOn(DIK_R) ||
                   (decide && menuSelection_ == TrophyChoice::Retry)) {
          if (decideSoundHandle_ != -1) system_->SoundPlaySE(decideSoundHandle_, 0.5f);
          choice_ = TrophyChoice::Retry;
        } else if (titleButton_->GetIsRelease() ||
                   system_->GetTriggerOn(DIK_1) ||
                   (decide && menuSelection_ == TrophyChoice::Title)) {
          if (decideSoundHandle_ != -1) system_->SoundPlaySE(decideSoundHandle_, 0.5f);
          choice_ = TrophyChoice::Title;
        }
}


void TrophyPart::Draw() {

	nextThemeButton_->Render();
	font_->RenderText(
		"お題を変えてリトライ",
		{ 640.0f, 160.0f }, 32.0f,
		BitmapFont::Align::Center, 5, { 0.0f,1.0f,1.0f,1.0f });

	sameThemeButton_->Render();
	font_->RenderText(
		"同じお題でリトライ",
		{ 640.0f, 340.0f }, 32.0f,
		BitmapFont::Align::Center, 5, { 0.0f,1.0f,1.0f,1.0f });

	titleButton_->Render();
	font_->RenderText(
		"タイトルへ戻る",
		{ 640.0f, 520.0f }, 32.0f,
		BitmapFont::Align::Center, 5, { 0.0f,1.0f,1.0f,1.0f });

#ifdef USE_IMGUI
	ImGui::Begin("Contest - Trophy");

	ImGui::Text("[Trophy]");
	ImGui::Separator();

	if (choice_ == TrophyChoice::None) {
		ImGui::Text("What do you want to do?");
		ImGui::Spacing();
		ImGui::Text("  SPACE : Next theme");
		ImGui::Text("  R     : Retry (same theme)");
		ImGui::Text("  1     : Back to title");
	} else {
		ImGui::Text("Transitioning...");
	}

	ImGui::End();
#endif
}

bool TrophyPart::IsFinished() const {
	return choice_ != TrophyChoice::None;
}

TrophyChoice TrophyPart::GetChoice() const {
	return choice_;
}

PartCameraTransform TrophyPart::GetCameraTransform() const
{
	return cameraTransform_;
}
