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

	// 当選者へ白ライトを向ける演出パターン
	enum class RevealPattern {
		Snap,  // 終盤までためてパッと当選者へ向く
		Slow,  // 全区間ゆっくり当選者へ向く（従来）
		Feint, // 一旦非当選者へ向かってフェイント→パッと当選者へ
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

	// 演出パターン（構築時にランダム抽選）
	RevealPattern revealPattern_ = RevealPattern::Slow;
	int decoyIdx_ = -1; // Feint時に一旦向かう非当選キャラのインデックス
	// パターン別タイミング（0〜1の正規化時間）
	float snapStart_ = 0.8f;   // Snap: ここまで揺らして待ち、以降で一気に向く
	float feintAimEnd_ = 0.5f; // Feint: ここまでで非当選者へ向かい切る
	float feintHoldEnd_ = 0.7f; // Feint: ここまで非当選者を見つめる（溜め）
	float afterRevealHold_ = 1.0f;
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

	static Vector3 NormalizeSafe(const Vector3& v);
	static Vector3 HsvToRgb(float h, float s, float v);
	// i番ライトの揺れ方向（timer_ベース・位相オフセット付き）
	Vector3 SwayDir(int i) const;
	// pos から target へ向かう正規化方向
	static Vector3 AimDir(const Vector3& pos, const Vector3& target);
	// 2方向の線形補間（正規化はしない）
	static Vector3 LerpDir(const Vector3& a, const Vector3& b, float t);
};
