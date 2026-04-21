#include "JudgingPart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"

JudgingPart::JudgingPart(kEngine* system, BitmapFont* font,
	const ScoreResult& scoreResult,
	const std::vector<JudgeCommentResult>& judgeCommentResults)
	: IContestPart(system, font)
	, scoreResult_(scoreResult)
	, judgeCommentResults_(judgeCommentResults) 
{
	// 審査員ごとのカメラ位置（後でImGuiで調整）
	cameraTransforms_[0] = { { 0.3f, 0.5f, -1.0f }, { 0.0f, 3.1415f, 0.0f } };
	cameraTransforms_[1] = { {  0.0f, 0.5f, -1.0f }, { 0.0f, 3.1415f, 0.0f } };
	cameraTransforms_[2] = { {  -0.3f, 0.5f, -1.0f }, { 0.0f, 3.1415f, 0.0f } };
}

void JudgingPart::Update() {
	if (isFinished_) return;

	// SPACEで次の審査員へ
	if (system_->GetTriggerOn(DIK_SPACE)) {
		currentJudgeIndex_++;
		if (currentJudgeIndex_ >= 3) {
			isFinished_ = true;
		}
	}
}

void JudgingPart::Draw() {

	if (currentJudgeIndex_ < (int)judgeCommentResults_.size()) {
		font_->RenderText(
			judgeCommentResults_[currentJudgeIndex_].comment,
			{ 640.0f, 100.0f }, 64.0f,
			BitmapFont::Align::Center);
	}

#ifdef USE_IMGUI
	ImGui::Begin("Contest - Judging");

	ImGui::Text("[Judging] Judge %d / 3", currentJudgeIndex_ + 1);
	ImGui::Separator();

	if (currentJudgeIndex_ < 3) {
		const auto& je = scoreResult_.judgeEvaluations[currentJudgeIndex_];

		if (!je.judgeId.empty()) {
			// 審査員名とタイトル
			ImGui::Text("Judge: %s (%s)", je.judgeId.c_str(),
				je.judgeTitle.c_str());

			// 個別★
			ImGui::Text("Star: %s (%d)",
				StarsToString(je.star).c_str(), je.star);

			// コメント
			if (currentJudgeIndex_ < (int)judgeCommentResults_.size()) {
				ImGui::Text("Comment: \"%s\"",
					judgeCommentResults_[currentJudgeIndex_].comment.c_str());
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Camera (Judge %d):", currentJudgeIndex_ + 1);
	ImGui::DragFloat3("Pos##judging",
		&cameraTransforms_[currentJudgeIndex_].position.x, 0.1f);
	ImGui::DragFloat3("Rot##judging",
		&cameraTransforms_[currentJudgeIndex_].rotation.x, 0.01f);

	ImGui::Spacing();
	if (!isFinished_) {
		ImGui::Text("Press SPACE for next judge");
	}

	ImGui::End();
#endif
}

bool JudgingPart::IsFinished() const {
	return isFinished_;
}

PartCameraTransform JudgingPart::GetCameraTransform() const
{
	if (currentJudgeIndex_ >= 0 && currentJudgeIndex_ < 3) {
		return cameraTransforms_[currentJudgeIndex_];
	}
	return cameraTransforms_[0];
}

std::string JudgingPart::StarsToString(int stars) {
	std::string result;
	for (int i = 0; i < 5; ++i) {
		result += (i < stars) ? "*" : "-";
	}
	return result;
}
