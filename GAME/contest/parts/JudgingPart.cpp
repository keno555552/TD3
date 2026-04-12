#include "JudgingPart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"

JudgingPart::JudgingPart(kEngine* system, BitmapFont* font,
	const ScoreResult& scoreResult,
	const std::vector<JudgeCommentResult>& judgeCommentResults)
	: IContestPart(system, font)
	, scoreResult_(scoreResult)
	, judgeCommentResults_(judgeCommentResults) {
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

std::string JudgingPart::StarsToString(int stars) {
	std::string result;
	for (int i = 0; i < 5; ++i) {
		result += (i < stars) ? "*" : "-";
	}
	return result;
}
