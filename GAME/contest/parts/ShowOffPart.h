#pragma once
#include "IContestPart.h"
#include "GAME/audience/AudienceData.h"
#include"GAME/font/BitmapFont.h"

/// <summary>
/// お披露目パートのサブステート
/// </summary>
enum class ShowOffStep {
	StageView,      /// モデルを映す
	AudienceReact,  /// 観客コメント表示
	TurnToJudges,   /// カメラが審査員側へ移動
};

/// <summary>
/// お披露目パート
/// ステージ上のモデル表示 → 観客コメント → 審査員側へカメラ移動
/// </summary>
class ShowOffPart : public IContestPart {
public:
	ShowOffPart(kEngine* system, BitmapFont* font,const AudienceResult& audienceResult);
	~ShowOffPart() override = default;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;

private:
	const AudienceResult& audienceResult_;
	ShowOffStep step_ = ShowOffStep::StageView;
	bool isFinished_ = false;
};
