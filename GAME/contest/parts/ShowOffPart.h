#pragma once
#include "IContestPart.h"
#include "GAME/audience/AudienceData.h"
#include "GAME/font/BitmapFont.h"
#include"Vector2.h"
#include <random>

enum class ShowOffStep {
	StageView,
	AudienceReact,
	TurnToJudges,
};

// ランダム表示用の構造体
struct RandomTextDisplay {
	Vector2 position;
	float size;
};

class ShowOffPart : public IContestPart {
public:
	ShowOffPart(kEngine* system, BitmapFont* font,
		const AudienceResult& audienceResult);
	~ShowOffPart() override = default;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;
	PartCameraTransform GetCameraTransform() const override;

private:
	const AudienceResult& audienceResult_;
	ShowOffStep step_ = ShowOffStep::StageView;
	bool isFinished_ = false;

	void GenerateRandomDisplays();

	// ざわ8個＋コメント3個の表示情報
	std::vector<RandomTextDisplay> zawaDisplays_;
	std::vector<RandomTextDisplay> commentDisplays_;

	// カメラ設定（ImGuiで調整用）
	PartCameraTransform cameraTransform_;
};