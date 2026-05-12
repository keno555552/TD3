#pragma once
#include "IContestPart.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/userData/UserData.h"
#include "GAME/contest/ui/StarChart.h"
#include"GAME/Object/DetailButton/DetailButton.h"

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

	std::unique_ptr<DetailButton>nextButton_;
	const ScoreResult& scoreResult_;
	const EarnedNickname& earnedNickname_;

	ResultStep step_ = ResultStep::StarsAndChart;
	bool isFinished_ = false;

	PartCameraTransform cameraTransform_;

	PartCameraTransform GetCameraTransform() const override;

	/// 五芒星レーダーチャート（3層構造）
	StarChart bgStar_;     /// 背景：全★5の最大星形（薄く表示）
	StarChart starChart_;  /// 中段：実値の星形
	StarChart fgStar_;     /// 前景：全★1の最小星形

	/// StarChartの表示を現在のステップに合わせて更新する
	void UpdateStarChart();

	/// 現在のステップまでの★項目を表示する（ImGui仮表示用）
	void DrawStarItem(const char* label, int star, bool visible) const;

	/// ★を文字列に変換
	static std::string StarsToString(int stars);
};
