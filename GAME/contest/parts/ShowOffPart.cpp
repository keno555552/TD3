#include "ShowOffPart.h"
#include "kEngine.h"

ShowOffPart::ShowOffPart(kEngine* system, BitmapFont* font, const AudienceResult& audienceResult)
	: IContestPart(system, font)
	, audienceResult_(audienceResult) {
}

void ShowOffPart::Update() {
	if (isFinished_) return;

	switch (step_) {
	case ShowOffStep::StageView:
		// SPACEで観客コメント表示へ
		if (system_->GetTriggerOn(DIK_SPACE)) {
			step_ = ShowOffStep::AudienceReact;
		}
		break;

	case ShowOffStep::AudienceReact:
		// SPACEで審査員側へカメラ移動（今は即遷移）
		if (system_->GetTriggerOn(DIK_SPACE)) {
			step_ = ShowOffStep::TurnToJudges;
		}
		break;

	case ShowOffStep::TurnToJudges:
		// TODO: カメラ移動完了判定（今は即完了）
		isFinished_ = true;
		break;
	}
}

void ShowOffPart::Draw() {
#ifdef USE_IMGUI
	ImGui::Begin("Contest - ShowOff");

	switch (step_) {
	case ShowOffStep::StageView:
		ImGui::Text("[ShowOff] Stage View");
		ImGui::Separator();
		ImGui::Text("Player model on stage");
		ImGui::Spacing();
		ImGui::Text("Press SPACE to continue");
		break;

	case ShowOffStep::AudienceReact:
		ImGui::Text("[ShowOff] Audience Reaction");
		ImGui::Separator();
		for (int i = 0; i < 3; ++i) {
			ImGui::Text("  Audience %d: %s", i + 1,
				audienceResult_.comments[i].c_str());
		}
		ImGui::Spacing();
		ImGui::Text("Press SPACE to continue");
		break;

	case ShowOffStep::TurnToJudges:
		ImGui::Text("[ShowOff] Camera moving to judges...");
		break;
	}

	ImGui::End();
#endif
}

bool ShowOffPart::IsFinished() const {
	return isFinished_;
}
