#pragma once
#include "IContestPart.h"
#include "GAME/audience/AudienceData.h"
#include "GAME/font/BitmapFont.h"
#include"GAME/Object/DetailButton/DetailButton.h"
#include"Vector2.h"
#include <random>

enum class ShowOffStep {
	StageView,
	AudienceReact,
	TurnToJudges,
};

/// ニコニコ風スクロールコメント用の構造体
struct ScrollingComment {
	Vector2 position;   /// 現在の描画位置
	float size;         /// フォントサイズ
	float speed;        /// スクロール速度（px/秒）
	float yPos;         /// Y座標（固定、ループ時に再利用）
};

enum class ZawaState {
	Wait,
	FadeIn,
	FadeOut
};

struct ZawaEffect {
	Vector2 position;
	float size;
	ZawaState state;
	float timer;
	float maxTime;
	float alpha;
};

class ShowOffPart : public IContestPart {
public:
	ShowOffPart(kEngine* system, BitmapFont* font,
		const AudienceResult& audienceResult);
	~ShowOffPart() override;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;
	PartCameraTransform GetCameraTransform() const override;

private:
	std::unique_ptr<DetailButton>nextButton_;
	const AudienceResult& audienceResult_;
	ShowOffStep step_ = ShowOffStep::StageView;
	bool isFinished_ = false;
	int decideSoundHandle_ = -1;

	/// スクロールコメントとざわの初期化
	void GenerateEffects();

	/// 画面外に出たコメントを右端からリスタートさせる
	void ResetScrollPosition(ScrollingComment& sc, std::mt19937& gen);

	/// ざわの新しいランダム位置を取得する（中央付近と他との被りを避ける）
	Vector2 GetRandomZawaPosition(std::mt19937& gen, float& outSize);

	/// ざわ8個のフェード情報
	std::vector<ZawaEffect> zawaEffects_;
	/// コメント3個のスクロール情報
	std::vector<ScrollingComment> commentScrolls_;

	/// ループ用の乱数生成器
	std::mt19937 rng_;

	/// 画面幅（スクロール範囲の基準）
	static constexpr float kScreenWidth = 1280.0f;
	/// 画面高さ
	static constexpr float kScreenHeight = 720.0f;
	/// 画面外マージン（右端からの出現位置オフセット）
	static constexpr float kSpawnMargin = 200.0f;

	// カメラ設定（ImGuiで調整用）
	PartCameraTransform cameraTransform_;
};