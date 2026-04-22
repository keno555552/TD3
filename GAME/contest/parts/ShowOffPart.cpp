#include "ShowOffPart.h"
#include "kEngine.h"

ShowOffPart::ShowOffPart(kEngine* system, BitmapFont* font,
	const AudienceResult& audienceResult)
	: IContestPart(system, font), audienceResult_(audienceResult) {

	// 初期カメラ位置（後でImGuiで調整）
	cameraTransform_.position = { 0.0f, 3.5f, -13.0f };
	cameraTransform_.rotation = { 0.15f, 0.0f, 0.0f };

	GenerateRandomDisplays();
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

	// ざわをランダム配置で表示
	for (const auto& zawa : zawaDisplays_) {
		font_->RenderText("ざわ・・・", zawa.position, zawa.size,
			BitmapFont::Align::Center);
	}

	// 観客コメントをランダム配置で表示
	for (int i = 0; i < 3; ++i) {
		if (i < (int)commentDisplays_.size() &&
			!audienceResult_.comments[i].empty()) {
			font_->RenderText(
				audienceResult_.comments[i],
				commentDisplays_[i].position,
				commentDisplays_[i].size,
				BitmapFont::Align::Center,
				5,{1.0f,0.0f,0.0f,1.0f});
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

void ShowOffPart::GenerateRandomDisplays() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> distX(70.0f, 1210.0f);
	std::uniform_real_distribution<float> distY(50.0f, 670.0f);
	std::uniform_int_distribution<int> distSize(0, 2);

	float sizes[] = { 32.0f, 48.0f, 64.0f };

	// ざわ8個
	zawaDisplays_.resize(8);
	for (auto& zawa : zawaDisplays_) {
		zawa.position = { distX(gen), distY(gen) };
		zawa.size = sizes[distSize(gen)];
	}

	// コメント3個
	commentDisplays_.resize(3);
	for (auto& comment : commentDisplays_) {
		comment.position = { distX(gen), distY(gen) };
		comment.size = sizes[distSize(gen)];
	}
}