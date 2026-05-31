#pragma once
#include "GAME/Object/DetailButton/DetailButton.h"
#include "IContestPart.h"

/// <summary>
/// トロフィーパートでの選択結果
/// </summary>
enum class TrophyChoice {
  None,      /// まだ選択していない
  NextTheme, /// 次のお題へ（SPACE）
  Retry,     /// 同じお題でリトライ（R）
  Title,     /// タイトルへ戻る（1）
};

/// <summary>
/// トロフィーパート
/// 保存するか選択 → リトライ/次のお題/タイトルへ
/// </summary>
class TrophyPart : public IContestPart {
public:
  TrophyPart(kEngine *system, BitmapFont *font, Vector3 playerTarget);
  ~TrophyPart() override;

  void Update() override;
  void Draw() override;
  bool IsFinished() const override;

  /// <summary>
  /// プレイヤーの選択結果を取得
  /// </summary>
  TrophyChoice GetChoice() const;

private:
  std::unique_ptr<DetailButton> titleButton_;
  std::unique_ptr<DetailButton> sameThemeButton_;
  std::unique_ptr<DetailButton> nextThemeButton_;
  std::unique_ptr<bool> isGoTitle_;
  PartCameraTransform GetCameraTransform() const override;

  PartCameraTransform cameraTransform_;

  TrophyChoice choice_ = TrophyChoice::None;

  TrophyChoice menuSelection_ = TrophyChoice::NextTheme;
  float menuInputCooldown_ = 0.0f;
  Vector2 prevMousePos_ = {0.0f, 0.0f};

  int decideSoundHandle_ = -1;
  int selectSoundHandle_ = -1;
};
