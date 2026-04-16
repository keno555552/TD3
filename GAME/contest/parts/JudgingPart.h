#pragma once
#include "IContestPart.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/judges/comment/JudgeCommentData.h"
#include <vector>

/// <summary>
/// 審査パート
/// 審査員1人ずつコメント＋個別★表示（総合は非表示）
/// </summary>
class JudgingPart : public IContestPart {
public:
	JudgingPart(kEngine* system, BitmapFont* font,
		const ScoreResult& scoreResult,
		const std::vector<JudgeCommentResult>& judgeCommentResults);
	~JudgingPart() override = default;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;

private:
	const ScoreResult& scoreResult_;
	const std::vector<JudgeCommentResult>& judgeCommentResults_;

	int currentJudgeIndex_ = 0;
	bool isFinished_ = false;

	/// ★を文字列に変換
	static std::string StarsToString(int stars);
};
