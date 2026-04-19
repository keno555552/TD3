#include "ShowOffPart.h"
#include "kEngine.h"

ShowOffPart::ShowOffPart(kEngine* system, BitmapFont* font,
	const AudienceResult& audienceResult)
	: IContestPart(system, font), audienceResult_(audienceResult) {

	// 初期カメラ位置（後でImGuiで調整）
	cameraTransform_.position = { 0.0f, 3.5f, -13.0f };
	cameraTransform_.rotation = { 0.15f, 0.0f, 0.0f };
}

void ShowOffPart::Update() {
	if (isFinished_) return;

	switch (step_) {
	case ShowOffStep::StageView:
		// SPACEで審査員側へカメラ移動
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

	font_->RenderText("ざわ・・・", { 100,100 }, 64, BitmapFont::Align::Left);
	font_->RenderText("ざわ・・・", { 200,10 }, 32, BitmapFont::Align::Left);
	font_->RenderText("ざわ・・・", { 1000,600 }, 48, BitmapFont::Align::Right);
	font_->RenderText("ざわ・・・", { 130,650 }, 64, BitmapFont::Align::Left);
	font_->RenderText("ざわ・・・", { 400,370 }, 32, BitmapFont::Align::Left);
	font_->RenderText("ざわ・・・", { 900,250 }, 64, BitmapFont::Align::Right);
	font_->RenderText("ざわ・・・", { 1200,200 }, 48, BitmapFont::Align::Right);
	font_->RenderText("// ざわ・・・", { 640,360 }, 48, BitmapFont::Align::Center);

	// 観客コメントを常に表示
	for (int i = 0; i < 3; ++i) {
		if (!audienceResult_.comments[i].empty()) {
			font_->RenderText(
				audienceResult_.comments[i],
				{ 640.0f, 100.0f + i * 80.0f }, 32.0f,
				BitmapFont::Align::Center);
		}
	}

#ifdef USE_IMGUI
	ImGui::Begin("Contest - ShowOff");

	ImGui::Text("[ShowOff] %s",
		step_ == ShowOffStep::StageView ? "Stage View" : "Turn To Judges");

	ImGui::Separator();
	for (int i = 0; i < 3; ++i) {
		ImGui::Text("  Audience %d: %s", i + 1,
			audienceResult_.comments[i].c_str());
	}

	ImGui::Separator();
	ImGui::Text("Camera:");
	ImGui::DragFloat3("Pos##showoff", &cameraTransform_.position.x, 0.1f);
	ImGui::DragFloat3("Rot##showoff", &cameraTransform_.rotation.x, 0.01f);

	ImGui::Spacing();
	ImGui::Text("Press SPACE to continue");

	ImGui::End();
#endif
}

bool ShowOffPart::IsFinished() const {
	return isFinished_;
}

PartCameraTransform ShowOffPart::GetCameraTransform() const {
	return cameraTransform_;
}