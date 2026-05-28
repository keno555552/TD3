#pragma once
#include "IContestPart.h"
#include "GAME/Object/DetailButton/DetailButton.h"
#include "Vector3.h"
#include <memory>
#include <string>
#include <vector>

class Light;

/// <summary>
/// 結果〜ランキングの間に挟む「誰が選ばれる？」演出パート
/// 3つのスポットライト（プレイヤー＋NPC×2）の位置はそのまま、
/// 向きをXZ軸で揺らし色を周期変化させ、決定ボタンで当選者へ全ライトを向ける
/// </summary>
class SuspensePart : public IContestPart {
public:
	SuspensePart(kEngine* system, BitmapFont* font,
		Light* lights[3], const Vector3 targetPositions[3],
		const std::vector<int>& winnerIndices);
	~SuspensePart() override;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;
	PartCameraTransform GetCameraTransform() const override;

private:
	enum class State {
		Suspense,
		Revealing,
		Done,
	};

	Light* lights_[3] = { nullptr, nullptr, nullptr };
	Vector3 targetPositions_[3];
	Vector3 lightOriginalDir_[3];
	Vector3 lightOriginalColor_[3];
	float originalIntensity_[3] = { 0.0f, 0.0f, 0.0f };

	// 演出中の各ライトの向き計算用にスタート方向を保持
	Vector3 revealStartDir_[3];

	std::vector<int> winnerIndices_;
	bool isWinner_[3] = { false, false, false };
	State state_ = State::Suspense;
	float timer_ = 0.0f;
	float revealTimer_ = 0.0f;
	float revealDuration_ = 2.5f;
	float afterRevealHold_ = 1.0f;
	float afterRevealTimer_ = 0.0f;

	std::unique_ptr<DetailButton> drawButton_;
	PartCameraTransform cameraTransform_;

	// サウンド
	int drumrollHandle_ = -1;
	int heartbeatHandle_ = -1;
	int spotlightSeHandle_ = -1;
	int cheersHandle_ = -1;

	static Vector3 NormalizeSafe(const Vector3& v);
	static Vector3 HsvToRgb(float h, float s, float v);
};
