#pragma once
#include "IContestPart.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/userData/UserData.h"
#include "GAME/contest/ui/StarChart.h"

/// <summary>
/// 結果パートのサブステート
/// </summary>
enum class ResultStep {
	StarsAndChart,      /// 五芒星＋★5項目を一気に表示
	Total,              /// ★合計
	RankAndNickname,    /// 総合ランク＋二つ名（同時表示）
};

/// <summary>
/// 結果パート
/// ★5項目 → ★合計 → 総合ランク＋二つ名
/// </summary>
class ResultPart : public IContestPart {
public:
	ResultPart(kEngine* system, BitmapFont* font,
		const ScoreResult& scoreResult,
		const EarnedNickname& earnedNickname);
	~ResultPart() override;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;

private:
	const ScoreResult& scoreResult_;
	const EarnedNickname& earnedNickname_;

	ResultStep step_ = ResultStep::StarsAndChart;
	bool isFinished_ = false;

	/// 五芒星レーダーチャート
	StarChart starChart_;

	/// StarChartの表示を現在のステップに合わせて更新する
	void UpdateStarChart();

	/// 現在のステップまでの★項目を表示する（ImGui仮表示用）
	void DrawStarItem(const char* label, int star, bool visible) const;

	/// ★を文字列に変換
	static std::string StarsToString(int stars);
};
