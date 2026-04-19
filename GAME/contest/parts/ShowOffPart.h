#pragma once
#include "IContestPart.h"
#include "GAME/audience/AudienceData.h"
#include "GAME/font/BitmapFont.h"

enum class ShowOffStep {
	StageView,
	AudienceReact,
	TurnToJudges,
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

	// カメラ設定（ImGuiで調整用）
	PartCameraTransform cameraTransform_;
};