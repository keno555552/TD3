#include "TrophyPart.h"
#include "kEngine.h"

TrophyPart::TrophyPart(kEngine* system, BitmapFont* font)
	: IContestPart(system, font) 
{
	cameraTransform_ = { { 0.0f, 2.0f, -5.0f }, { 0.0f, 0.0f, 0.0f } };

}

void TrophyPart::Update() {
	if (choice_ != TrophyChoice::None) return;

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
}

void TrophyPart::Draw() {
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
