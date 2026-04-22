#include "TrophyPart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"

TrophyPart::TrophyPart(kEngine* system, BitmapFont* font)
	: IContestPart(system, font) 
{
	cameraTransform_ = { { 0.0f, 1.2f, -3.0f }, { 0.12f, 0.0f, 0.0f } };

	nextThemeButton_ = std::make_unique<DetailButton>(system);
	nextThemeButton_->SetButton({ 640.0f, 180.0f }, 400.0f, 80.0f);

	sameThemeButton_ = std::make_unique<DetailButton>(system);
	sameThemeButton_->SetButton({ 640.0f, 360.0f }, 400.0f, 80.0f);

	titleButton_ = std::make_unique<DetailButton>(system);
	titleButton_->SetButton({ 640.0f, 540.0f }, 400.0f, 80.0f);
}

void TrophyPart::Update() {
	if (choice_ != TrophyChoice::None) return;

	nextThemeButton_->Update();
	sameThemeButton_->Update();
	titleButton_->Update();

	// SPACE: 次のお題へ
	if (system_->GetTriggerOn(DIK_SPACE)) {
		choice_ = TrophyChoice::NextTheme;
	}
	// R: 同じお題でリトライ
	else if (system_->GetTriggerOn(DIK_R)) {
		choice_ = TrophyChoice::Retry;
	}
	// 1: タイトルへ戻る
	else if (system_->GetTriggerOn(DIK_1)) {
		choice_ = TrophyChoice::Title;
	}

	if (nextThemeButton_->GetIsPress()) {
		choice_ = TrophyChoice::NextTheme;
	}

	if (sameThemeButton_->GetIsPress()) {
		choice_ = TrophyChoice::Retry;
	}

	if (titleButton_->GetIsPress()){
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
