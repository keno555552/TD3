#include "ResultPart.h"
#include "kEngine.h"

ResultPart::ResultPart(kEngine* system, BitmapFont* font,
	const ScoreResult& scoreResult,
	const EarnedNickname& earnedNickname)
	: IContestPart(system, font)
	, scoreResult_(scoreResult)
	, earnedNickname_(earnedNickname) {

	// 五芒星レーダーチャート初期化（画面右側に配置）
	// HD: 1280x720 → 右側中心あたり (900, 360)
	starChart_.Initialize(system_, 900.0f, 360.0f);

	// 最初から全5項目をセット
	starChart_.SetStars(
		scoreResult_.starThemeMatch,
		scoreResult_.starImpact,
		scoreResult_.starCommitment,
		scoreResult_.starEfficiency,
		scoreResult_.starJudgeBonus);
}

ResultPart::~ResultPart() {
	starChart_.Cleanup();
}

void ResultPart::Update() {
	if (isFinished_) return;

	// SPACEで次のステップへ
	if (system_->GetTriggerOn(DIK_SPACE)) {
		switch (step_) {
		case ResultStep::StarsAndChart:
			step_ = ResultStep::Total;
			break;
		case ResultStep::Total:
			step_ = ResultStep::RankAndNickname;
			break;
		case ResultStep::RankAndNickname:
			isFinished_ = true;
			break;
		}
	}
}

void ResultPart::Draw() {
	// 五芒星レーダーチャート描画
	starChart_.Draw();

#ifdef USE_IMGUI
	ImGui::Begin("Contest - Result");

	ImGui::Text("[Result]");
	ImGui::Separator();

	// ★5項目を全部表示
	DrawStarItem("Theme Match", scoreResult_.starThemeMatch, true);
	DrawStarItem("Impact", scoreResult_.starImpact, true);
	DrawStarItem("Commitment", scoreResult_.starCommitment, true);
	DrawStarItem("Efficiency", scoreResult_.starEfficiency, true);
	DrawStarItem("Judge Eval", scoreResult_.starJudgeBonus, true);

	// ★合計
	if (step_ >= ResultStep::Total) {
		ImGui::Separator();
		ImGui::Text("Total Stars: %d / 25", scoreResult_.totalStars);
	}

	// ランク＋二つ名（同時表示）
	if (step_ >= ResultStep::RankAndNickname) {
		ImGui::Separator();
		ImGui::Text("Overall Rank: %s", scoreResult_.overallRank.c_str());

		if (earnedNickname_.isRare) {
			ImGui::Text("Nickname: [RARE] %s",
				earnedNickname_.nickname.c_str());
		} else {
			ImGui::Text("Nickname: %s",
				earnedNickname_.nickname.c_str());
		}
	}

	ImGui::Spacing();
	if (!isFinished_) {
		ImGui::Text("Press SPACE to continue");
	}

	ImGui::End();
#endif
}

bool ResultPart::IsFinished() const {
	return isFinished_;
}

void ResultPart::UpdateStarChart() {
	// 現在は全項目一気に表示なので常に全値をセット
	starChart_.SetStars(
		scoreResult_.starThemeMatch,
		scoreResult_.starImpact,
		scoreResult_.starCommitment,
		scoreResult_.starEfficiency,
		scoreResult_.starJudgeBonus);
}

void ResultPart::DrawStarItem(const char* label, int star, bool visible) const {
	if (visible) {
		ImGui::Text("  %-14s: %s (%d)", label,
			StarsToString(star).c_str(), star);
	}
}

std::string ResultPart::StarsToString(int stars) {
	std::string result;
	for (int i = 0; i < 5; ++i) {
		result += (i < stars) ? "*" : "-";
	}
	return result;
}
