#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <array>

class TravelScene : public BaseScene {
public:
  TravelScene(kEngine *system);
  ~TravelScene();

  void Update() override;
  void Draw() override;

private:
  /*ライト
  ------------------------------*/
  Light *light1_ = nullptr;

  /* カメラ
  ------------------------------*/
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = true;

  /* フェード
  ------------------------------*/
  Fade fade_;
  bool isStartTransition_ = false;

  // フェードアウト完了後の遷移先
  SceneOutcome nextOutcome_ = SceneOutcome::NONE;

  /* モデル描画用
  ------------------------------*/
  // モデルハンドル
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> modModelHandles_{};
  // 各部位オブジェクト
  std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> modObjects_{};

  void SetupModObjects();
  void SetupPartObject(ModBodyPart part, const std::string &path);
  void SetupHierarchy();
  void SetupInitialLayout();
  void UpdateModObjects();
  void DrawModObjects();

  /* プレイヤー移動用変数
  ------------------------------*/
  float moveX_ = 0.0f;
  float velocityX_ = 0.0f;
  float speedDamping_ = 0.95f;

  bool isFirstStep_ = true;   // 最初の一歩か
  bool lastStepLeft_ = false; // 前回が左脚か

  float leftStepPower_ = 0.12f;  // 左脚の前進力
  float rightStepPower_ = 0.12f; // 右脚の前進力

  float alternateBonus_ = 1.0f;   // 交互入力ボーナス
  float sameSidePenalty_ = 0.35f; // 同じ脚連打の前進率

  float legSwingAngle_ = 0.5f;    // 脚を振る角度
  float leftLegSwing_ = 0.0f;     // 左脚の現在見た目角度
  float rightLegSwing_ = 0.0f;    // 右脚の現在見た目角度
  float legSwingDamping_ = 0.85f; // 脚振り戻り

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();
};