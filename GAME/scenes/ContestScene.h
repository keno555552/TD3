#pragma once
#include "GAME/effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/prompt/PromptData.h"
#include "GAME/score/ScoreCalculator.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/actor/ModBody.h"

#include "GAME/nickname/NicknameManager.h"
#include "GAME/nickname/NicknameData.h"
#include "GAME/userData/UserDataManager.h"

#include "GAME/judges/comment/JudgeCommentManager.h"
#include "GAME/judges/comment/JudgeCommentData.h"

#include "GAME/audience/AudienceManager.h"
#include "GAME/audience/AudienceData.h"

#include "GAME/font/BitmapFont.h"

#include "GAME/contest/parts/IContestPart.h"
#include <memory>

/// <summary>
/// コンテストの進行フェーズ
/// </summary>
enum class ContestPhase {
	ShowOff,  /// お披露目
	Judging,  /// 審査
	Result,   /// 結果
	Trophy,   /// トロフィー・選択
};

class ContestScene : public BaseScene {
public:
	ContestScene(kEngine* system);
	~ContestScene();

	void Update() override;
	void Draw() override;

private:
	// ライト
	Light* light1_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;
	DebugCamera* debugCamera_ = nullptr;
	Camera* usingCamera_ = nullptr;
	bool useDebugCamera_ = false;

	// フェード
	Fade fade_;
	bool isStartTransition_ = false;
	SceneOutcome nextOutcome_ = SceneOutcome::NONE;

	// スコア・評価データ
	ScoreResult scoreResult_{};
	bool isScoreCalculated_ = false;

	// 二つ名
	NicknameTableData nicknameTable_{};
	UserDataManager* userDataManager_ = nullptr;
	EarnedNickname earnedNickname_{};

	// フォント
	BitmapFont bitmapFont_;

	// 審査員コメント
	JudgeCommentTable judgeCommentTable_{};
	std::vector<JudgeCommentResult> judgeCommentResults_;

	// 観客コメント
	AudienceCommentData audienceCommentData_{};
	AudienceResult audienceResult_{};

	// パート管理（ステートパターン）
	ContestPhase phase_ = ContestPhase::ShowOff;
	std::unique_ptr<IContestPart> currentPart_;

	/// <summary>
	/// 次のパートへ遷移する
	/// </summary>
	void AdvancePhase();

	/// <summary>
	/// 指定フェーズのパートを生成する
	/// </summary>
	std::unique_ptr<IContestPart> CreatePart(ContestPhase phase);

	/// <summary>
	/// トロフィーパートの選択結果を処理する
	/// </summary>
	void HandleTrophyChoice();

	/// <summary>
	/// 使用するカメラを設定・更新する
	/// </summary>
	void CameraPart();
};
