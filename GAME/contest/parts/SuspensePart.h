#pragma once
#include "IContestPart.h"
#include "GAME/Object/DetailButton/DetailButton.h"
#include "GAME/font/BitmapFont.h"
#include "GAME/actor/ModCustomizedBodyActor.h"
#include "GAME/effect/ConfettiParticle.h"
#include "kEngine.h"
#include <vector>
#include <memory>
#include <string>

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
		const std::vector<int>& winnerIndices,
		ModCustomizedBodyActor* actors[3],
		Light* envLight,
		Camera* camera);
	~SuspensePart() override;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;
	PartCameraTransform GetCameraTransform() const override;
	bool UseCustomCameraControl() const override { return true; }

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

	// Snapパターンのタイミング（0〜1の正規化時間）
	// 0.88 = 2.2秒まで揺れ続け、残り0.3秒で一気に向く
	float snapStart_ = 0.88f;   
	
	float afterRevealHold_ = 5.0f;
	float afterRevealTimer_ = 0.0f;

	// ライトの揺れパラメータ（Suspense / 演出待機中で共有）
	float swayAmp_ = 0.45f;
	float swayFreq_ = 2.2f;

	std::unique_ptr<DetailButton> drawButton_;
	PartCameraTransform cameraTransform_;

	// サウンド
	int drumrollHandle_ = -1;
	int heartbeatHandle_ = -1;
	int spotlightSeHandle_ = -1;
	int cheersHandle_ = -1;
	float easeTimer_ = 0.0f; // Dolly zoom用

	ModCustomizedBodyActor* actors_[3] = { nullptr, nullptr, nullptr };
	Light* envLight_ = nullptr;
	float envLightOriginalIntensity_ = 0.0f;
	Camera* camera_ = nullptr;
	std::unique_ptr<ConfettiParticle> confetti_;
	bool hasSpawnedConfetti_ = false;

	// Flash Effect
	float flashTimer_ = 0.0f;
	float flashDuration_ = 0.5f;
	bool isFlashing_ = false;

	// Camera target index for cuts
	int currentCutIndex_ = -1;
	float cutTimer_ = 0.0f;
	float nextCutTime_ = 1.5f;

	static Vector3 NormalizeSafe(const Vector3& v);
	static Vector3 HsvToRgb(float h, float s, float v);
	// i番ライトの揺れ方向（timer_ベース・位相オフセット付き）
	Vector3 SwayDir(int i) const;
	// pos から target へ向かう正規化方向
	static Vector3 AimDir(const Vector3& pos, const Vector3& target);
	// 2方向の線形補間（正規化はしない）
	static Vector3 LerpDir(const Vector3& a, const Vector3& b, float t);
	// イージング
	static float EaseOutExpo(float t);
};
