#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <array>
#include <memory>
#include <string>

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

  bool useDebugCamera_ = false;

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

  std::array<ModBody, static_cast<size_t>(ModBodyPart::Count)> modBodies_{};

  std::array<Vector3, static_cast<size_t>(ModBodyPart::Count)>
      bodyJointOffsets_{};

  std::unique_ptr<ModBodyCustomizeData> customizeData_ = nullptr;

  void SetupModObjects();
  void SetupPartObject(ModBodyPart part, const std::string &path);
  void SetupHierarchy();
  void SetupInitialLayout();
  void UpdateModObjects();
  void DrawModObjects();
  void SetupBodyJointOffsets();
  void LoadCustomizeData();
  void UpdateChildRootsFromBody();

  /* プレイヤー移動用変数
  ------------------------------*/
  float moveX_ = 0.0f;
  float velocityX_ = 0.0f;

  // 検証用仮パラメータ
  // float stability_ = 1.0f;  // 高いほど安定
  // float accel_ = 1.0f;      // 高いほど一歩が強い
  // float baseSpeed_ = 0.12f; // 一歩の基礎前進量
  // float inertia_ = 0.90f;   // 高いほど速度が残る

  // float autoSpeed_ = 0.08f;
  // float steerPower_ = 0.01f;
  // float returnForce_ = 0.003f;

  // float drift_ = 0.003f;

  float inertia_ = 0.96f; // 高いほど流れが残る

  float moveY_ = 0.0f;     // プレイヤーの高さ
  float velocityY_ = 0.0f; // 縦速度
  float gravity_ = 0.006f; // 重力
  float groundY_ = 0.0f;   // 地面の高さ
  bool isGrounded_ = true; // 接地中か
  float jumpRatio_ = 3.0f;

  /* プレイヤー見た目用変数
　------------------------------*/
  float bodyTilt_ = 0.0f;
  float bodyTiltVelocity_ = 0.0f;

  // 姿勢の基準・制限
  float maxForwardTilt_ = -0.60f; // 前傾の限界
  float maxBackwardTilt_ = 0.20f; // 後傾の限界

  // 入力による姿勢寄与
  float legDiffTiltPower_ = 0.08f; // 左右差による微調整

  // 脚の目標姿勢
  float legKickAngle_ = -0.55f;   // 押している間の蹴り姿勢
  float legRecoverAngle_ = 0.85f; // 離している間の回収姿勢
  float legFollowPower_ = 0.18f;  // 目標角へ寄る強さ
  float legMaxSpeed_ = 0.12f;     // 脚の角速度上限

  //===============================
  // 疑似ラグドール移動用
  //===============================
  float leftLegBend_ = 0.0f;       // 左脚の曲げ量
  float rightLegBend_ = 0.0f;      // 右脚の曲げ量
  float leftLegBendSpeed_ = 0.0f;  // 左脚の曲げ速度
  float rightLegBendSpeed_ = 0.0f; // 右脚の曲げ速度

  float bodyStretch_ = 0.0f;      // 体の伸ばし量
  float bodyStretchSpeed_ = 0.0f; // 体の伸ばし速度

  float jointDamping_ = 0.88f; // 関節の減衰

  float leftLegPrevBend_ = 0.0f;  // 前フレーム左脚曲げ量
  float rightLegPrevBend_ = 0.0f; // 前フレーム右脚曲げ量

  float pushPower_ = 0.25f;   // 脚が伸びた瞬間の前進力
  float groundAssist_ = 1.0f; // 地面押し補正

  bool debugForceTilt_ = false;
  float debugTiltValue_ = 0.0f;

  float minKickPower_ = 0.45f; // 単押しでも出る最低キック

  // 姿勢評価用
  float idealRunTilt_ = -0.12f;    // 走りやすい理想姿勢
  float postureTolerance_ = 0.32f; // この範囲なら良姿勢扱い

  // push時の姿勢反動
  float pushTiltKick_ = -0.2f; // 蹴った瞬間に少し前へ倒す

  // 簡易接地判定用
  float footContactBendThreshold_ =
      0.15f;                        // この角度以上なら足が地面に触れている扱い
  float groundedKickFactor_ = 1.0f; // 接地しているときの蹴り係数

  //===============================
  // 接地中の押し込み蓄積
  //===============================
  float leftDriveAccum_ = 0.0f;
  float rightDriveAccum_ = 0.0f;

  // 直前にどちらの脚で蹴ったか
  // -1 : 左, 0 : なし, 1 : 右
  int lastKickSide_ = 0;

  // 接地中に押し込んだ量の溜まりやすさ
  float driveBuildScale_ = 2.8f;

  // 接地が切れたあと・入力が抜けたあとの減衰
  float driveDecay_ = 0.01f;

  // これ未満の蓄積は蹴りとして扱わない
  float minDriveToKick_ = 0.035f;

  // 左右踏み替えボーナス / 同じ側連打ペナルティ
  float alternateKickBonus_ = 1.18f;
  float sameSideKickPenalty_ = 0.88f;

  // 蓄積の上限
  float maxDriveAccum_ = 0.22f;

  // 同時押しペナ
  float bothHoldBuildPenalty_ = 0.75f;

  //===============================
  // ホールド時間管理
  //===============================
  float leftHoldTime_ = 0.0f;
  float rightHoldTime_ = 0.0f;

  // これ以上押していないと蹴り不成立
  float minHoldTimeToKick_ = 0.08f;

  // 押している間に1フレームで使う drive 量
  float driveUsePerFrame_ = 0.06f;

  // hold成立後、実際に push に変換するときの倍率
  float drivePushScale_ = 3.0f;

  float leftLegPrevBendSpeed_ = 0.0f;
  float rightLegPrevBendSpeed_ = 0.0f;

  //===============================
  // ゴール判定
  //===============================
  float goalX_ = 20.0f;
  bool isGoalReached_ = false;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();
};