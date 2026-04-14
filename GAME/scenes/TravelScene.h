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
  /// <summary>
  /// 更新に必要な部位がそろっているかチェック
  /// </summary>
  /// <returns></returns>
  bool HasRequiredParts() const;

  /// <summary>
  /// 前フレーム保存
  /// </summary>
  void SavePreviousFrameState();

  /// <summary>
  /// 制限時間
  /// </summary>
  /// <param name="deltaTime"></param>
  void UpdateTimeLimit(float deltaTime);

  /// <summary>
  /// キーのホールドを取得
  /// </summary>
  /// <param name="leftNowInput"></param>
  /// <param name="rightNowInput"></param>
  /// <param name="deltaTime"></param>
  void UpdateHoldState(bool leftNowInput, bool rightNowInput, float deltaTime);

  /// <summary>
  /// 脚の曲げ状態と速度の更新
  /// </summary>
  /// <param name="leftNowInput"></param>
  /// <param name="rightNowInput"></param>
  void UpdateLegBendState(bool leftNowInput, bool rightNowInput);

  /// <summary>
  /// 入力や姿勢から移動を更新
  /// </summary>
  /// <param name="leftNowInput"></param>
  /// <param name="rightNowInput"></param>
  void UpdateMovementState(bool leftNowInput, bool rightNowInput);

  /// <summary>
  /// 見た目反映(アニメーション)
  /// </summary>
  void ApplyVisualState();

  /// <summary>
  /// シーン遷移
  /// </summary>
  void UpdateSceneTransition();

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
  void ApplyCustomizeToMovementParam();
  float ComputeLegHeightOffset() const;

  /* プレイヤー移動用変数
  ------------------------------*/
  float moveX_ = 0.0f;
  float velocityX_ = 0.0f;

  float inertia_ = 0.96f; // 高いほど流れが残る

  float moveY_ = 0.0f;      // プレイヤーの高さ
  float velocityY_ = 0.0f;  // 縦速度
  float gravity_ = 0.0045f; // 重力
  float groundY_ = 0.0f;    // 地面の高さ
  bool isGrounded_ = true;  // 接地中か
  float jumpRatio_ = 3.0f;  // ジャンプ係数

  /* プレイヤー姿勢用変数
　------------------------------*/
  float bodyTilt_ = 0.0f;         // 体の傾き
  float bodyTiltVelocity_ = 0.0f; // 傾き速度

  // 姿勢の基準・制限
  float maxForwardTilt_ = -0.60f; // 前傾の限界
  float maxBackwardTilt_ = 0.20f; // 後傾の限界

  // ImGui表示用
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

  //===============================
  // 改造によるパラメータ
  //===============================
  struct CharacterFeatures {
    int headCount = 0;
    int armCount = 0;

    float centerOfMassY = 0.0f;
    float asymmetry = 0.0f;
    float lowestPoint = 0.0f;
  };

  struct TravelTuning {
    float runPower = 1.0f;
    float maxSpeed = 1.0f;
    float stability = 1.0f;
    float lift = 1.0f;
    float turnResponse = 1.0f;
    float strideScale = 1.0f;
  };

  CharacterFeatures features_;
  TravelTuning tuning_;

  bool useCustomizeMove_ = true;

  bool requireReleaseAfterLandLeft_ = false;
  bool requireReleaseAfterLandRight_ = false;

  // 地面
  std::unique_ptr<Object> ground_ = nullptr;
  uint32_t groundModelHandle_ = 0;

  // 制限時間
  float timeLimit_ = 30.0f;
  float travelTimeLimit_ = 1.0f;
  bool isTimeUp_ = false;

  // 追加オブジェクト
  std::vector<Object *> extraObjects_;
  std::vector<int> extraParentIds_;
  std::unordered_map<int, ObjectPart *> fixedPartIdToPart_;

  std::vector<ModControlPointSnapshot> controlPointSnapshots_;

  float leftLegReturnScale_ = 1.0f;
  float rightLegReturnScale_ = 1.0f;
  float timingWindowScale_ = 1.0f;
  float recoveryAssist_ = 1.0f;

  bool isRecoveringFromTilt_ = false;
  float tiltRecoveryTimer_ = 0.0f;

  float gaitTiltTarget_ = 0.0f;

  float landTimer_ = 999.0f;

  float headSizeScale_ = 1.0f;

  bool leftPrevInput_ = false;
  bool rightPrevInput_ = false;

  float torsoStabilityScale_ = 1.0f;
  float torsoTiltResistance_ = 1.0f;
  float torsoAgilityScale_ = 1.0f;

  float torsoSizeScale_ = 1.0f;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();

private:
  /// <summary>
  /// 構造を特徴量に変換
  /// </summary>
  void BuildFeaturesFromCustomizeData();

  /// <summary>
  ///
  /// </summary>
  void BuildExtraVisualParts();

  void UpdateExtraVisualParts();
  void DrawExtraVisualParts();
  void ClearExtraVisualParts();

  void UpdatePartRootsFromControlPoints();

  const ModControlPointData *GetControlPoints() const;

  float GetControlPointRadius(ModControlPointRole role) const;

  float GetSnapshotRadius(ModBodyPart ownerPart, int localRole) const;

  struct SegmentVisual {
    Vector3 root;
    float angleZ;
    float length;
    float thickness;
  };

  bool BuildSegmentFromSnapshot(ModBodyPart partType, int partId,
                                SegmentVisual &out);

private:
  // 沼
  std::unique_ptr<Object> swamp_ = nullptr;
  uint32_t swampModelHandle_ = 0;

  float swampZ_ = 0.0f;          // 沼の中心Z
  float swampWidth_ = 1.0f;      // 沼の幅
  float swampJumpScale_ = 0.1f;  // 沼に入った時の減速率
  float swampSlowScale_ = 0.33f; // 沼に入った時の減速率
  bool isInSwamp_ = false;

  // タイミング
  enum class KickFeedbackType {
    None,
    Good,
    Perfect,
  };

private:
  KickFeedbackType kickFeedbackType_ = KickFeedbackType::None;
  float kickFeedbackTimer_ = 0.0f;
};