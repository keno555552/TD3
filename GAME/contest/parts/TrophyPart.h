#pragma once
#include "IContestPart.h"

/// <summary>
/// トロフィーパートでの選択結果
/// </summary>
enum class TrophyChoice {
	None,       /// まだ選択していない
	NextTheme,  /// 次のお題へ（SPACE）
	Retry,      /// 同じお題でリトライ（R）
	Title,      /// タイトルへ戻る（1）
};

/// <summary>
/// トロフィーパート
/// 保存するか選択 → リトライ/次のお題/タイトルへ
/// </summary>
class TrophyPart : public IContestPart {
public:
	TrophyPart(kEngine* system, BitmapFont* font);
	~TrophyPart() override = default;

	void Update() override;
	void Draw() override;
	bool IsFinished() const override;

	/// <summary>
	/// プレイヤーの選択結果を取得
	/// </summary>
	TrophyChoice GetChoice() const;

private:
	PartCameraTransform GetCameraTransform() const override;

	PartCameraTransform cameraTransform_;

	TrophyChoice choice_ = TrophyChoice::None;
};
