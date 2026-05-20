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
	std::unique_ptr<DetailButton>nextButton_;
	const AudienceResult& audienceResult_;
	ShowOffStep step_ = ShowOffStep::StageView;
	bool isFinished_ = false;

	/// スクロールコメントの初期化
	void GenerateScrollingComments();

	/// 画面外に出たコメントを右端からリスタートさせる
	void ResetScrollPosition(ScrollingComment& sc, std::mt19937& gen);

	/// ざわ8個＋コメント3個のスクロール情報
	std::vector<ScrollingComment> zawaScrolls_;
	std::vector<ScrollingComment> commentScrolls_;

	/// ループ用の乱数生成器
	std::mt19937 rng_;

	/// 画面幅（スクロール範囲の基準）
	static constexpr float kScreenWidth = 1280.0f;
	/// 画面外マージン（右端からの出現位置オフセット）
	static constexpr float kSpawnMargin = 200.0f;

	// カメラ設定（ImGuiで調整用）
	PartCameraTransform cameraTransform_;
};