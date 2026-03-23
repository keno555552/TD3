#pragma once
#include "GAME/effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/prompt/PromptData.h"
#include "GAME/score/ScoreCalculator.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/actor/ModBody.h"

class ContestScene : public BaseScene {
public:
  ContestScene(kEngine *system);
  ~ContestScene();

  void Update() override;
  void Draw() override;

private:
  // 仮ライト
  Light *light1_ = nullptr;

  // カメラ
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = false;

  // フェード
  Fade fade_;
  bool isStartTransition_ = false;

  // フェードアウト完了後の遷移先
  SceneOutcome nextOutcome_ = SceneOutcome::NONE;

  ScoreResult scoreResult_{};
  bool isScoreCalculated_ = false;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();
};
