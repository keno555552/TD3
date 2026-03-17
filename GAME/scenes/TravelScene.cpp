#include "TravelScene.h"
#include <cmath>

namespace {
/*    enum class を配列添字に変換するための補助関数   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

/*   成分ごとのスケール適用   */
Vector3 ScaleByRatio(const Vector3 &base, const Vector3 &ratio) {
  return {base.x * ratio.x, base.y * ratio.y, base.z * ratio.z};
}

constexpr float kFaceRightY = 1.57f;
} // namespace

TravelScene::TravelScene(kEngine *system) {
  system_ = system;

  //===============================
  // ライト
  //===============================
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  //===============================
  // カメラ
  //===============================
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();

  // デバッグカメラ初期位置
  debugCamera_->SetTranslate({0.0f, 0.0f, -48.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());

  // 通常カメラ初期位置
  camera_->SetTranslate({0.0f, 0.0f, -48.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());

  usingCamera_ = camera_;
  system_->SetCamera(usingCamera_);

  //===============================
  // 3Dオブジェクト
  //===============================

  // 各部位のハンドルとオブジェクト配列を初期化
  modModelHandles_.fill(0);
  modObjects_.fill(nullptr);

  // 改造用の各部位オブジェクトをセットアップ
  SetupModObjects();

  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  LoadCustomizeData();

  modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetParam().length = 1.25f;
  modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetParam().length = 1.25f;

  modBodies_[ToIndex(ModBodyPart::LeftForeArm)].GetParam().length = 1.20f;
  modBodies_[ToIndex(ModBodyPart::RightForeArm)].GetParam().length = 1.20f;

  modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetParam().length = 1.65f;
  modBodies_[ToIndex(ModBodyPart::RightThigh)].GetParam().length = 1.65f;

  modBodies_[ToIndex(ModBodyPart::LeftShin)].GetParam().length = 1.55f;
  modBodies_[ToIndex(ModBodyPart::RightShin)].GetParam().length = 1.55f;

  UpdateChildRootsFromBody();

  //===============================
  // 2D
  //===============================
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

TravelScene::~TravelScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  // 各部位オブジェクトを解放
  for (auto &object : modObjects_) {
    delete object;
    object = nullptr;
  }

  system_->RemoveLight(light1_);

  delete light1_;
}

void TravelScene::Update() {

  //===============================
  // カメラ更新
  //===============================

  CameraPart();

  //===============================
  // プレイヤー移動
  //===============================

  //===============================
  // 疑似ラグドール移動テスト
  //===============================
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  if (body == nullptr || neck == nullptr || head == nullptr ||
      leftUpperArm == nullptr || leftForeArm == nullptr ||
      rightUpperArm == nullptr || rightForeArm == nullptr ||
      leftThigh == nullptr || leftShin == nullptr || rightThigh == nullptr ||
      rightShin == nullptr) {
    return;
  }

  // 前フレーム値保存
  leftLegPrevBend_ = leftLegBend_;
  rightLegPrevBend_ = rightLegBend_;

  leftLegPrevBendSpeed_ = leftLegBendSpeed_;
  rightLegPrevBendSpeed_ = rightLegBendSpeed_;

  //--------------------------------
  // 入力：押すと蹴り出す、離すと回収する
  //--------------------------------
  bool leftNowInput = system_->GetIsPush(DIK_A);
  bool rightNowInput = system_->GetIsPush(DIK_D);

  const float deltaTime = system_->GetDeltaTime();

  bool bothInput = leftNowInput && rightNowInput;

  if (leftNowInput) {
    leftHoldTime_ += deltaTime;
  } else {
    leftHoldTime_ = 0.0f;
  }

  if (rightNowInput) {
    rightHoldTime_ += deltaTime;
  } else {
    rightHoldTime_ = 0.0f;
  }

  float leftTargetLegAngle = leftNowInput ? legKickAngle_ : legRecoverAngle_;
  float rightTargetLegAngle = rightNowInput ? legKickAngle_ : legRecoverAngle_;

  leftLegBendSpeed_ += (leftTargetLegAngle - leftLegBend_) * legFollowPower_;
  rightLegBendSpeed_ += (rightTargetLegAngle - rightLegBend_) * legFollowPower_;

  leftLegBendSpeed_ =
      std::clamp(leftLegBendSpeed_, -legMaxSpeed_, legMaxSpeed_);
  rightLegBendSpeed_ =
      std::clamp(rightLegBendSpeed_, -legMaxSpeed_, legMaxSpeed_);

  // SPACEで体を伸ばす（ジャンプというより反動）
  // if (system_->GetIsPush(DIK_SPACE)) {
  //  bodyStretchSpeed_ += stretchPower_ * accel_;
  //}

  //--------------------------------
  // 減衰
  //--------------------------------
  leftLegBendSpeed_ *= jointDamping_;
  rightLegBendSpeed_ *= jointDamping_;
  bodyStretchSpeed_ *= jointDamping_;

  //--------------------------------
  // 値更新
  //--------------------------------
  leftLegBend_ += leftLegBendSpeed_;
  rightLegBend_ += rightLegBendSpeed_;
  bodyStretch_ += bodyStretchSpeed_;

  // 曲げ量は大きくなりすぎないように軽く制限
  leftLegBend_ = std::clamp(leftLegBend_, -0.8f, 1.0f);
  rightLegBend_ = std::clamp(rightLegBend_, -0.8f, 1.0f);
  bodyStretch_ = std::clamp(bodyStretch_, -0.5f, 1.0f);

  //--------------------------------
  // 簡易足接地判定
  //--------------------------------
  bool leftFootContact =
      isGrounded_ && (leftLegBend_ >= footContactBendThreshold_);
  bool rightFootContact =
      isGrounded_ && (rightLegBend_ >= footContactBendThreshold_);

  //--------------------------------
  // 姿勢更新
  //--------------------------------

  float forwardTorque = 0.0f;
  float backwardTorque = 0.0f;
  float pushTorque = 0.0f;

  //--------------------------------
  // 姿勢更新
  // まだ脚差では target を作らない
  //--------------------------------
  float legDiffTilt = (leftLegBend_ - rightLegBend_) * legDiffTiltPower_;

  //==============================
  // 脚回収による後傾トルク
  // 「離している間ずっと」ではなく
  // 「前に戻す加速」が出た時だけ反動を入れる
  //==============================

  if (!leftNowInput) {
    float leftRecoveryAccel = leftLegBendSpeed_ - leftLegPrevBendSpeed_;

    if (leftRecoveryAccel > 0.0f) {
      float torque = leftRecoveryAccel * 0.1f;
      backwardTorque += torque;
    }
  }

  if (!rightNowInput) {
    float rightRecoveryAccel = rightLegBendSpeed_ - rightLegPrevBendSpeed_;

    if (rightRecoveryAccel > 0.0f) {
      float torque = rightRecoveryAccel * 0.1f;
      backwardTorque += torque;
    }
  }

  float targetTilt = 0.0f;

  // 両押し：前傾
  if (bothInput) {
    targetTilt = -0.15f;
  }

  float tiltError = targetTilt - bodyTilt_;
  bodyTiltVelocity_ += tiltError * 0.02f;

  float restoreTorque = -bodyTilt_ * 0.01f;
  bodyTiltVelocity_ += restoreTorque;

  //--------------------------------
  // 姿勢の良し悪しを作る
  //--------------------------------
  // float tiltAbs = std::abs(bodyTilt_);
  // float badPosture = std::clamp(tiltAbs / 0.40f, 0.0f, 1.0f);

  //// 良い姿勢ほど前進しやすい
  // float forwardRate = 1.0f - badPosture;

  //// 悪い姿勢ほど上に逃げやすい
  // float upwardRate = 0.35f + badPosture * 0.65f;

  float postureError = std::abs(bodyTilt_ - idealRunTilt_);
  float badPosture = std::clamp(postureError / postureTolerance_, 0.0f, 1.0f);

  // 良い姿勢ほど前進しやすい
  float forwardRate = 1.0f - badPosture;

  // 悪い姿勢ほど上に逃げやすい
  float upwardRate = 0.30f + badPosture * 0.70f;

  //--------------------------------
  // 押している間に drive を使って前進する
  //--------------------------------
  bool leftHoldEnough = (leftHoldTime_ >= minHoldTimeToKick_);
  bool rightHoldEnough = (rightHoldTime_ >= minHoldTimeToKick_);

  float leftPushCandidate = 0.0f;
  float rightPushCandidate = 0.0f;

  bool useLeftPush = false;
  bool useRightPush = false;
  float adoptedDriveUse = 0.0f;

  if (leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      leftDriveAccum_ >= minDriveToKick_ && leftHoldEnough) {

    float sequenceBonus = 1.0f;

    if (lastKickSide_ == 1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == -1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);

    leftPushCandidate = driveUse * drivePushScale_ * pushPower_ *
                        groundAssist_ * groundedKickFactor_ * 2.0f *
                        sequenceBonus;
  }

  if (rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      rightDriveAccum_ >= minDriveToKick_ && rightHoldEnough) {
    float sequenceBonus = 1.0f;
    if (lastKickSide_ == -1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == 1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = (std::min)(rightDriveAccum_, driveUsePerFrame_);

    rightPushCandidate = driveUse * drivePushScale_ * pushPower_ *
                         groundAssist_ * groundedKickFactor_ * 2.0f *
                         sequenceBonus;
  }

  float totalPush = 0.0f;

  if (leftPushCandidate >= rightPushCandidate && leftPushCandidate > 0.0f) {
    totalPush = leftPushCandidate;
    useLeftPush = true;
    adoptedDriveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);
    lastKickSide_ = -1;

    leftDriveAccum_ -= adoptedDriveUse;
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_);

    // 同時押しで反対側が溜まり続けるのを少し抑える
    if (rightNowInput) {
      rightDriveAccum_ *= 0.85f;
    }

  } else if (rightPushCandidate > leftPushCandidate &&
             rightPushCandidate > 0.0f) {
    totalPush = rightPushCandidate;
    useRightPush = true;
    adoptedDriveUse = (std::min)(rightDriveAccum_, driveUsePerFrame_);
    lastKickSide_ = 1;

    rightDriveAccum_ -= adoptedDriveUse;
    rightDriveAccum_ = (std::max)(0.0f, rightDriveAccum_);

    if (leftNowInput) {
      leftDriveAccum_ *= 0.85f;
    }
  }

  if (totalPush > 0.0f) {
    //==============================
    // 体の前傾で前進効率を強く変える
    //==============================
    float tiltForwardFactor =
        std::clamp((-bodyTilt_ - 0.02f) * 3.5f, 0.0f, 1.5f);

    //==============================
    // 採用された脚が前にあるかどうかで push の向きを補正する
    //==============================
    float kickLegBend = 0.0f;
    if (useLeftPush) {
      kickLegBend = leftLegBend_;
    } else if (useRightPush) {
      kickLegBend = rightLegBend_;
    }

    // 0.0 = 蹴り姿勢側 / 1.0 = 回収姿勢側
    float kickLegForwardness =
        1.0f - std::clamp((kickLegBend - legKickAngle_) /
                              (legRecoverAngle_ - legKickAngle_),
                          0.0f, 1.0f);

    // 脚が前にあるほど前向きの蹴りになりやすい
    float footForwardBonus = 0.75f + kickLegForwardness * 0.75f;
    float footUpwardSuppress = 1.15f - kickLegForwardness * 0.55f;
    float groundBoost = isGrounded_ ? 1.2f : 0.6f;

    float pushX = totalPush *
                  (0.40f + forwardRate * 0.50f + tiltForwardFactor * 0.75f) *
                  footForwardBonus * groundBoost;

    float upwardSuppressByTilt =
        std::clamp(1.0f - tiltForwardFactor * 0.25f, 0.75f, 1.0f);

    float pushY = totalPush * jumpRatio_ * upwardRate * upwardSuppressByTilt *
                  footUpwardSuppress;

    // 同時押しは「耐える」寄りにして、前進効率を下げる
    // if (leftNowInput && rightNowInput) {
    //  pushX *= 0.65f;
    //  pushY *= 0.85f;
    //}

    velocityX_ += pushX;
    velocityY_ += pushY;

    // 蓄積した分だけ姿勢反動を速度として入れる
    // bodyTiltVelocity_ += pushTiltKick_ * adoptedDriveUse * 1.8f;

    // pushTorque = pushTiltKick_ * adoptedDriveUse * 2.0f;

    if (bothInput) {
      pushTorque = pushTiltKick_ * adoptedDriveUse * 2.0f;
    } else {
      pushTorque = 0.0f;
    }

    Logger::Log("---- HOLD DRIVE ----");
    Logger::Log("bodyTilt: %.5f", bodyTilt_);
    Logger::Log("legDiffTilt: %.5f", legDiffTilt);
    Logger::Log("tiltForwardFactor: %.5f", tiltForwardFactor);
    Logger::Log("kickLegBend: %.5f", kickLegBend);
    Logger::Log("kickLegForwardness: %.5f", kickLegForwardness);
    Logger::Log("footForwardBonus: %.5f", footForwardBonus);
    Logger::Log("footUpwardSuppress: %.5f", footUpwardSuppress);
    Logger::Log("forwardRate: %.5f", forwardRate);
    Logger::Log("upwardRate: %.5f", upwardRate);
    Logger::Log("leftDriveAccum: %.5f", leftDriveAccum_);
    Logger::Log("rightDriveAccum: %.5f", rightDriveAccum_);
    Logger::Log("leftHoldEnough: %d", leftHoldEnough ? 1 : 0);
    Logger::Log("rightHoldEnough: %d", rightHoldEnough ? 1 : 0);
    Logger::Log("leftPushCandidate: %.5f", leftPushCandidate);
    Logger::Log("rightPushCandidate: %.5f", rightPushCandidate);
    Logger::Log("useLeftPush: %d", useLeftPush ? 1 : 0);
    Logger::Log("useRightPush: %d", useRightPush ? 1 : 0);
    Logger::Log("totalPush: %.5f", totalPush);
    Logger::Log("pushX: %.5f", pushX);
    Logger::Log("pushY: %.5f", pushY);
    Logger::Log("bodyTiltVelocity: %.5f", bodyTiltVelocity_);
  }

  // 蹴りにならなかった側は少しずつ減衰
  if (!(leftNowInput && isGrounded_ && velocityY_ <= 0.0f)) {
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_ - driveDecay_);
  }

  if (!(rightNowInput && isGrounded_ && velocityY_ <= 0.0f)) {
    rightDriveAccum_ = (std::max)(0.0f, rightDriveAccum_ - driveDecay_);
  }

  if (pushTorque < 0.0f) {
    forwardTorque += -pushTorque;
  } else {
    backwardTorque += pushTorque;
  }

  float netTorque = backwardTorque - forwardTorque;
  bodyTiltVelocity_ += netTorque;

  Logger::Log("---- TILT TORQUE ----");
  Logger::Log("forwardTorque : %.5f", forwardTorque);
  Logger::Log("backwardTorque : %.5f", backwardTorque);
  Logger::Log("pushTorque : %.5f", pushTorque);
  Logger::Log("netTorque : %.5f", netTorque);
  Logger::Log("bodyTiltVelocity : %.5f", bodyTiltVelocity_);
  Logger::Log("bodyTilt : %.5f", bodyTilt_);

  // 減衰
  bodyTiltVelocity_ *= 0.80f;

  // 更新
  bodyTilt_ += bodyTiltVelocity_;

  // 限界角で止める
  bodyTilt_ = std::clamp(bodyTilt_, maxForwardTilt_, maxBackwardTilt_);

  // デバッグ：姿勢固定
  if (debugForceTilt_) {
    bodyTilt_ = debugTiltValue_;
  }

  //--------------------------------
  // 接地中に押し込み量を蓄積する
  //--------------------------------
  float leftExtendAmount = 0.0f;
  float rightExtendAmount = 0.0f;

  float leftRecoverAmount = 0.0f;
  float rightRecoverAmount = 0.0f;

  // 接地中に「ちゃんと押し込めている量」を溜める
  if (leftNowInput && isGrounded_ && velocityY_ <= 0.0f) {
    leftExtendAmount = (std::max)(0.0f, leftLegPrevBend_ - leftLegBend_);

    leftRecoverAmount = std::clamp((leftLegPrevBend_ - legKickAngle_) /
                                       (legRecoverAngle_ - legKickAngle_),
                                   0.0f, 1.0f);

    float buildScale = driveBuildScale_;
    if (leftNowInput && rightNowInput) {
      buildScale *= bothHoldBuildPenalty_;
    }

    float leftDriveGain = leftExtendAmount *
                          (minKickPower_ + leftRecoverAmount * 0.70f) *
                          buildScale;

    leftDriveAccum_ += leftDriveGain;
    leftDriveAccum_ = std::clamp(leftDriveAccum_, 0.0f, maxDriveAccum_);
  }

  if (rightNowInput && isGrounded_ && velocityY_ <= 0.0f) {
    rightExtendAmount = (std::max)(0.0f, rightLegPrevBend_ - rightLegBend_);

    rightRecoverAmount = std::clamp((rightLegPrevBend_ - legKickAngle_) /
                                        (legRecoverAngle_ - legKickAngle_),
                                    0.0f, 1.0f);

    float buildScale = driveBuildScale_;
    if (leftNowInput && rightNowInput) {
      buildScale *= bothHoldBuildPenalty_;
    }

    float rightDriveGain = rightExtendAmount *
                           (minKickPower_ + rightRecoverAmount * 0.70f) *
                           buildScale;

    rightDriveAccum_ += rightDriveGain;
    rightDriveAccum_ = std::clamp(rightDriveAccum_, 0.0f, maxDriveAccum_);
  }

  //// 蹴り発生条件
  // bool leftKickTrigger = (prevLeftInput_ && !leftNowInput && isGrounded_);

  // bool rightKickTrigger = (prevRightInput_ && !rightNowInput && isGrounded_);

  // bool leftHoldEnough = (leftHoldTimeBeforeRelease >= minHoldTimeToKick_);
  // bool rightHoldEnough = (rightHoldTimeBeforeRelease >= minHoldTimeToKick_);

  // float leftPushCandidate = 0.0f;
  // float rightPushCandidate = 0.0f;

  // bool useLeftPush = false;
  // bool useRightPush = false;
  // float adoptedDrive = 0.0f;

  // if (leftKickTrigger && leftDriveAccum_ >= minDriveToKick_ &&
  // leftHoldEnough) {
  //   float sequenceBonus = 1.0f;
  //   if (lastKickSide_ == 1) {
  //     sequenceBonus = alternateKickBonus_;
  //   } else if (lastKickSide_ == -1) {
  //     sequenceBonus = sameSideKickPenalty_;
  //   }

  //  leftPushCandidate = leftDriveAccum_ * pushPower_ * groundAssist_ *
  //                      groundedKickFactor_ * 2.0f * sequenceBonus;
  //}

  // if (rightKickTrigger && rightDriveAccum_ >= minDriveToKick_ &&
  //     rightHoldEnough) {
  //   float sequenceBonus = 1.0f;
  //   if (lastKickSide_ == -1) {
  //     sequenceBonus = alternateKickBonus_;
  //   } else if (lastKickSide_ == 1) {
  //     sequenceBonus = sameSideKickPenalty_;
  //   }

  //  rightPushCandidate = rightDriveAccum_ * pushPower_ * groundAssist_ *
  //                       groundedKickFactor_ * 2.0f * sequenceBonus;
  //}

  // float totalPush = 0.0f;

  // if (leftPushCandidate >= rightPushCandidate && leftPushCandidate > 0.0f) {
  //   totalPush = leftPushCandidate;
  //   adoptedDrive = leftDriveAccum_;
  //   useLeftPush = true;
  //   lastKickSide_ = -1;

  //  // 片側だけ残すと次回の同時押しで爆発するので両方消す
  //  leftDriveAccum_ = 0.0f;
  //  rightDriveAccum_ = 0.0f;

  //} else if (rightPushCandidate > leftPushCandidate &&
  //           rightPushCandidate > 0.0f) {
  //  totalPush = rightPushCandidate;
  //  adoptedDrive = rightDriveAccum_;
  //  useRightPush = true;
  //  lastKickSide_ = 1;

  //  // 片側だけ残すと次回の同時押しで爆発するので両方消す
  //  leftDriveAccum_ = 0.0f;
  //  rightDriveAccum_ = 0.0f;
  //}

  // if (totalPush > 0.0f) {
  //   float pushX = totalPush * forwardRate;
  //   float pushY = totalPush * jumpRatio_ * upwardRate;

  //  velocityX_ += pushX;
  //  velocityY_ += pushY;

  //  // 蓄積した分だけ姿勢反動
  //  bodyTilt_ += pushTiltKick_ * adoptedDrive * 6.0f;

  //  Logger::Log("---- HOLD KICK ----");
  //  Logger::Log("leftFootContact: %d", leftFootContact ? 1 : 0);
  //  Logger::Log("rightFootContact: %d", rightFootContact ? 1 : 0);
  //  Logger::Log("leftKickTrigger: %d", leftKickTrigger ? 1 : 0);
  //  Logger::Log("rightKickTrigger: %d", rightKickTrigger ? 1 : 0);
  //  Logger::Log("leftDriveAccum: %.5f", leftDriveAccum_);
  //  Logger::Log("rightDriveAccum: %.5f", rightDriveAccum_);
  //  Logger::Log("leftPushCandidate: %.5f", leftPushCandidate);
  //  Logger::Log("rightPushCandidate: %.5f", rightPushCandidate);
  //  Logger::Log("useLeftPush: %d", useLeftPush ? 1 : 0);
  //  Logger::Log("useRightPush: %d", useRightPush ? 1 : 0);
  //  Logger::Log("forwardRate: %.5f", forwardRate);
  //  Logger::Log("upwardRate: %.5f", upwardRate);
  //  Logger::Log("totalPush: %.5f", totalPush);
  //  Logger::Log("leftHoldTimeBeforeRelease: %.5f", leftHoldTimeBeforeRelease);
  //  Logger::Log("rightHoldTimeBeforeRelease: %.5f",
  //  rightHoldTimeBeforeRelease); Logger::Log("leftHoldEnough: %d",
  //  leftHoldEnough ? 1 : 0); Logger::Log("rightHoldEnough: %d",
  //  rightHoldEnough ? 1 : 0); Logger::Log("pushX: %.5f", pushX);
  //  Logger::Log("pushY: %.5f", pushY);
  //}

  bool wasGrounded = isGrounded_;
  //--------------------------------
  // 速度減衰
  //--------------------------------
  velocityX_ *= inertia_;
  moveX_ += velocityX_;

  velocityY_ -= gravity_;
  moveY_ += velocityY_;

  if (moveY_ <= groundY_) {
    isGrounded_ = true;
    moveY_ = groundY_;
    velocityY_ = 0.0f;
  } else {
    isGrounded_ = false;
  }

  leftFootContact = isGrounded_ && (leftLegBend_ >= footContactBendThreshold_);
  rightFootContact =
      isGrounded_ && (rightLegBend_ >= footContactBendThreshold_);

  //--------------------------------
  // 自然に元へ戻る
  //--------------------------------
  // leftLegBend_ *= 0.99f;
  // rightLegBend_ *= 0.99f;
  bodyStretch_ *= 0.90f;

  //--------------------------------
  // 見た目反映
  //--------------------------------
  body->mainPosition.transform.translate.x = moveX_;
  body->mainPosition.transform.translate.y = moveY_;

  body->mainPosition.transform.rotate.x = 0.0f;
  body->mainPosition.transform.rotate.y = 1.57f;
  body->mainPosition.transform.rotate.z = bodyTilt_;
  body->mainPosition.transform.scale.x = 1.0f;
  body->mainPosition.transform.scale.y =
      (std::max)(0.65f, 1.0f - bodyStretch_ * 0.2f);
  body->mainPosition.transform.scale.z = 1.0f;

  // まず全部リセット
  neck->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  head->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  leftUpperArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  leftForeArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  rightUpperArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightForeArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  leftThigh->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  leftShin->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  rightThigh->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightShin->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  // 頭
  head->mainPosition.transform.rotate.x =
      (leftLegBend_ - rightLegBend_) * -0.15f;

  // 腕
  leftUpperArm->mainPosition.transform.rotate.x = -rightLegBend_ * 0.6f;
  rightUpperArm->mainPosition.transform.rotate.x = -leftLegBend_ * 0.6f;

  leftForeArm->mainPosition.transform.rotate.x =
      leftUpperArm->mainPosition.transform.rotate.x - 0.45f;
  rightForeArm->mainPosition.transform.rotate.x =
      rightUpperArm->mainPosition.transform.rotate.x - 0.45f;

  const float thighBase = 0.10f;
  const float shinBase = -0.05f;

  leftThigh->mainPosition.transform.rotate.x = thighBase - leftLegBend_ * 0.70f;
  rightThigh->mainPosition.transform.rotate.x =
      thighBase - rightLegBend_ * 0.70f;

leftShin->mainPosition.transform.rotate.x =
      leftThigh->mainPosition.transform.rotate.x + 0.45f;
  rightShin->mainPosition.transform.rotate.x =
      rightThigh->mainPosition.transform.rotate.x + 0.45f;

  UpdateChildRootsFromBody();

  // mesh側の回転は使わない
  if (!neck->objectParts_.empty()) {
    neck->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!head->objectParts_.empty()) {
    head->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftUpperArm->objectParts_.empty()) {
    leftUpperArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftForeArm->objectParts_.empty()) {
    leftForeArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightUpperArm->objectParts_.empty()) {
    rightUpperArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightForeArm->objectParts_.empty()) {
    rightForeArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftThigh->objectParts_.empty()) {
    leftThigh->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftShin->objectParts_.empty()) {
    leftShin->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightThigh->objectParts_.empty()) {
    rightThigh->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightShin->objectParts_.empty()) {
    rightShin->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }

  UpdateModObjects();

  //--------------------------------
  // ゴール到達で次シーンへ
  //--------------------------------
  if (!isGoalReached_ && moveX_ >= goalX_) {
    isGoalReached_ = true;

    if (!fade_.IsBusy()) {
      fade_.StartFadeOut();
      isStartTransition_ = true;
      nextOutcome_ = SceneOutcome::NEXT;
    }
  }

  // Nキーで評価シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_N)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::NEXT;
  }

  // リトライ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_RETURN)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETRY;
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = nextOutcome_;
  }
}

void TravelScene::Draw() {

  DrawModObjects();

#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("TravelScene");
  ImGui::End();
#endif

#ifdef USE_IMGUI
  ImGui::Begin("TravelDebug");

  //==============================
  // 位置・速度
  //==============================
  ImGui::Text("MoveX : %.3f", moveX_);
  ImGui::Text("MoveY : %.3f", moveY_);
  ImGui::Text("VelocityX : %.3f", velocityX_);
  ImGui::Text("VelocityY : %.3f", velocityY_);

  //==============================
  // 姿勢確認
  //==============================
  float legDiffTilt = (leftLegBend_ - rightLegBend_) * legDiffTiltPower_;

  float postureError = std::abs(bodyTilt_ - idealRunTilt_);
  float badPosture = std::clamp(postureError / postureTolerance_, 0.0f, 1.0f);
  float forwardRate = 1.0f - badPosture;
  float upwardRate = 0.30f + badPosture * 0.70f;

  ImGui::Separator();
  ImGui::Text("BodyTilt : %.4f", bodyTilt_);
  ImGui::Text("LegDiffTilt : %.4f", legDiffTilt);

  ImGui::Text("LeftLegBend : %.4f", leftLegBend_);
  ImGui::Text("RightLegBend : %.4f", rightLegBend_);

  ImGui::Text("ForwardRate : %.4f", forwardRate);
  ImGui::Text("UpwardRate : %.4f", upwardRate);

  ImGui::Text("LeftDriveAccum : %.4f", leftDriveAccum_);
  ImGui::Text("RightDriveAccum : %.4f", rightDriveAccum_);

  ImGui::Text("LeftHoldTime : %.4f", leftHoldTime_);
  ImGui::Text("RightHoldTime : %.4f", rightHoldTime_);

  ImGui::Text("BodyTiltVelocity : %.4f", bodyTiltVelocity_);
  ImGui::Text("LeftLegBendSpeed : %.4f", leftLegBendSpeed_);
  ImGui::Text("RightLegBendSpeed : %.4f", rightLegBendSpeed_);

  ImGui::Checkbox("Force Tilt", &debugForceTilt_);
  ImGui::SliderFloat("Tilt Value", &debugTiltValue_, -0.4f, 0.4f);

  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}

void TravelScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}

void TravelScene::SetupModObjects() {
  SetupPartObject(ModBodyPart::Body, "GAME/resources/modBody/body/body.obj");
  SetupPartObject(ModBodyPart::Neck, "GAME/resources/modBody/neck/neck.obj");
  SetupPartObject(ModBodyPart::Head, "GAME/resources/modBody/head/head.obj");

  SetupPartObject(ModBodyPart::LeftUpperArm,
                  "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj");
  SetupPartObject(ModBodyPart::LeftForeArm,
                  "GAME/resources/modBody/leftForeArm/leftForeArm.obj");
  SetupPartObject(ModBodyPart::RightUpperArm,
                  "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj");
  SetupPartObject(ModBodyPart::RightForeArm,
                  "GAME/resources/modBody/rightForeArm/rightForeArm.obj");

  SetupPartObject(ModBodyPart::LeftThigh,
                  "GAME/resources/modBody/leftThighs/leftThighs.obj");
  SetupPartObject(ModBodyPart::LeftShin,
                  "GAME/resources/modBody/leftShin/leftShin.obj");
  SetupPartObject(ModBodyPart::RightThigh,
                  "GAME/resources/modBody/rightThighs/rightThighs.obj");
  SetupPartObject(ModBodyPart::RightShin,
                  "GAME/resources/modBody/rightShin/rightShin.obj");

  SetupHierarchy();
  SetupInitialLayout();
  SetupBodyJointOffsets();
}

/*   指定した部位のObjectを1つ生成する   */
void TravelScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  const size_t index = ToIndex(part);

  modModelHandles_[index] = system_->SetModelObj(path);

  modObjects_[index] = new Object;
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);
  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();

  modBodies_[index].Initialize(modObjects_[index], part);
}

/*   部位同士の親子関係を設定する   */
void TravelScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body == nullptr) {
    return;
  }

  ObjectPart *bodyRoot = &body->mainPosition;

  modObjects_[ToIndex(ModBodyPart::Neck)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;

  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ = bodyRoot;

  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->followObject_ = bodyRoot;

  modObjects_[ToIndex(ModBodyPart::LeftThigh)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftShin)]->followObject_ = bodyRoot;

  modObjects_[ToIndex(ModBodyPart::RightThigh)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightShin)]->followObject_ = bodyRoot;
}

/*   各部位の初期配置を設定する   */
void TravelScene::SetupInitialLayout() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  const Vector3 baseRotate = {0.0f, 0.0f, 0.0f};

  body->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
  body->mainPosition.transform.rotate = baseRotate;

  neck->mainPosition.transform.translate = {0.0f, 1.0f, 0.0f};
  neck->mainPosition.transform.rotate = baseRotate;

  head->mainPosition.transform.translate = {0.0f, 1.0f, 0.0f};
  head->mainPosition.transform.rotate = baseRotate;

  leftUpperArm->mainPosition.transform.translate = {-1.25f, 1.0f, 0.0f};
  leftUpperArm->mainPosition.transform.rotate = baseRotate;

  leftForeArm->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  leftForeArm->mainPosition.transform.rotate = baseRotate;

  rightUpperArm->mainPosition.transform.translate = {1.25f, 1.0f, 0.0f};
  rightUpperArm->mainPosition.transform.rotate = baseRotate;

  rightForeArm->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  rightForeArm->mainPosition.transform.rotate = baseRotate;

  leftThigh->mainPosition.transform.translate = {-0.5f, -1.25f, 0.0f};
  leftThigh->mainPosition.transform.rotate = baseRotate;

  leftShin->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  leftShin->mainPosition.transform.rotate = baseRotate;

  rightThigh->mainPosition.transform.translate = {0.5f, -1.25f, 0.0f};
  rightThigh->mainPosition.transform.rotate = baseRotate;

  rightShin->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  rightShin->mainPosition.transform.rotate = baseRotate;
}

/*   各部位Objectの更新をまとめて行う   */
void TravelScene::UpdateModObjects() {
  for (size_t i = 0; i < modObjects_.size(); ++i) {
    if (modObjects_[i] != nullptr) {
      modBodies_[i].Apply(modObjects_[i]);
    }
  }

  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Update(usingCamera_);
    }
  }
}

void TravelScene::DrawModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Draw();
    }
  }
}

void TravelScene::SetupBodyJointOffsets() {
  for (auto &offset : bodyJointOffsets_) {
    offset = {0.0f, 0.0f, 0.0f};
  }

  bodyJointOffsets_[ToIndex(ModBodyPart::Neck)] = {0.0f, 1.0f, 0.0f};

  bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)] = {-1.25f, 1.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)] = {1.25f, 1.0f, 0.0f};

  bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)] = {-0.5f, -1.25f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)] = {0.5f, -1.25f, 0.0f};
}

void TravelScene::LoadCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  bodyJointOffsets_ = customizeData_->bodyJointOffsets;

  for (size_t i = 0; i < modBodies_.size(); ++i) {
    modBodies_[i].SetParam(customizeData_->partParams[i]);
  }
}

// void TravelScene::UpdateChildRootsFromBody() {
//   const Vector3 bodyScale =
//       modBodies_[ToIndex(ModBodyPart::Body)].GetVisualScaleRatio();
//
//   const Vector3 neckJoint =
//       ScaleByRatio(bodyJointOffsets_[ToIndex(ModBodyPart::Neck)], bodyScale);
//
//   const Vector3 leftUpperArmJoint = ScaleByRatio(
//       bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)], bodyScale);
//
//   const Vector3 rightUpperArmJoint = ScaleByRatio(
//       bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)], bodyScale);
//
//   const Vector3 leftThighJoint = ScaleByRatio(
//       bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)], bodyScale);
//
//   const Vector3 rightThighJoint = ScaleByRatio(
//       bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)], bodyScale);
//
//   const Vector3 neckScale =
//       modBodies_[ToIndex(ModBodyPart::Neck)].GetVisualScaleRatio();
//   const Vector3 leftUpperArmScale =
//       modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetVisualScaleRatio();
//   const Vector3 rightUpperArmScale =
//       modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetVisualScaleRatio();
//   const Vector3 leftThighScale =
//       modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetVisualScaleRatio();
//   const Vector3 rightThighScale =
//       modBodies_[ToIndex(ModBodyPart::RightThigh)].GetVisualScaleRatio();
//
//   // 1段目
//   modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition.transform.translate =
//       neckJoint;
//
//   modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
//       ->mainPosition.transform.translate = leftUpperArmJoint;
//
//   modObjects_[ToIndex(ModBodyPart::RightUpperArm)]
//       ->mainPosition.transform.translate = rightUpperArmJoint;
//
//   modObjects_[ToIndex(ModBodyPart::LeftThigh)]
//       ->mainPosition.transform.translate = leftThighJoint;
//
//   modObjects_[ToIndex(ModBodyPart::RightThigh)]
//       ->mainPosition.transform.translate = rightThighJoint;
//
//   // 2段目も body 基準で直接置く
//   modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.transform.translate =
//   {
//       neckJoint.x, neckJoint.y + neckScale.y, neckJoint.z};
//
//   modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
//       ->mainPosition.transform.translate = {
//       leftUpperArmJoint.x, leftUpperArmJoint.y - leftUpperArmScale.y,
//       leftUpperArmJoint.z};
//
//   modObjects_[ToIndex(ModBodyPart::RightForeArm)]
//       ->mainPosition.transform.translate = {
//       rightUpperArmJoint.x, rightUpperArmJoint.y - rightUpperArmScale.y,
//       rightUpperArmJoint.z};
//
//   modObjects_[ToIndex(ModBodyPart::LeftShin)]
//       ->mainPosition.transform.translate = {
//       leftThighJoint.x, leftThighJoint.y - leftThighScale.y,
//       leftThighJoint.z};
//
//   modObjects_[ToIndex(ModBodyPart::RightShin)]
//       ->mainPosition.transform.translate = {
//       rightThighJoint.x, rightThighJoint.y - rightThighScale.y,
//       rightThighJoint.z};
// }

void TravelScene::UpdateChildRootsFromBody() {
  const Vector3 bodyScale =
      modBodies_[ToIndex(ModBodyPart::Body)].GetVisualScaleRatio();

  const Vector3 neckJoint =
      ScaleByRatio(bodyJointOffsets_[ToIndex(ModBodyPart::Neck)], bodyScale);

  const Vector3 leftUpperArmJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)], bodyScale);

  const Vector3 rightUpperArmJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)], bodyScale);

  const Vector3 leftThighJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)], bodyScale);

  const Vector3 rightThighJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)], bodyScale);

  const Vector3 neckScale =
      modBodies_[ToIndex(ModBodyPart::Neck)].GetVisualScaleRatio();
  const Vector3 leftUpperArmScale =
      modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetVisualScaleRatio();
  const Vector3 rightUpperArmScale =
      modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetVisualScaleRatio();
  const Vector3 leftThighScale =
      modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetVisualScaleRatio();
  const Vector3 rightThighScale =
      modBodies_[ToIndex(ModBodyPart::RightThigh)].GetVisualScaleRatio();

  // 1段目
  modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition.transform.translate =
      neckJoint;

  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
      ->mainPosition.transform.translate = leftUpperArmJoint;

  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]
      ->mainPosition.transform.translate = rightUpperArmJoint;

  modObjects_[ToIndex(ModBodyPart::LeftThigh)]
      ->mainPosition.transform.translate = leftThighJoint;

  modObjects_[ToIndex(ModBodyPart::RightThigh)]
      ->mainPosition.transform.translate = rightThighJoint;

  // 頭
  modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.transform.translate = {
      neckJoint.x, neckJoint.y + neckScale.y, neckJoint.z};

  // 上腕・腿の現在回転を使って、肘・膝の位置を計算
  const float leftUpperArmRotX = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
                                     ->mainPosition.transform.rotate.x;
  const float rightUpperArmRotX =
      modObjects_[ToIndex(ModBodyPart::RightUpperArm)]
          ->mainPosition.transform.rotate.x;
  const float leftThighRotX = modObjects_[ToIndex(ModBodyPart::LeftThigh)]
                                  ->mainPosition.transform.rotate.x;
  const float rightThighRotX = modObjects_[ToIndex(ModBodyPart::RightThigh)]
                                   ->mainPosition.transform.rotate.x;

  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
      ->mainPosition.transform.translate = {
      leftUpperArmJoint.x,
      leftUpperArmJoint.y - std::cos(leftUpperArmRotX) * leftUpperArmScale.y,
      leftUpperArmJoint.z - std::sin(leftUpperArmRotX) * leftUpperArmScale.y};

  modObjects_[ToIndex(ModBodyPart::RightForeArm)]
      ->mainPosition.transform.translate = {
      rightUpperArmJoint.x,
      rightUpperArmJoint.y - std::cos(rightUpperArmRotX) * rightUpperArmScale.y,
      rightUpperArmJoint.z -
          std::sin(rightUpperArmRotX) * rightUpperArmScale.y};

  modObjects_[ToIndex(ModBodyPart::LeftShin)]
      ->mainPosition.transform.translate = {
      leftThighJoint.x,
      leftThighJoint.y - std::cos(leftThighRotX) * leftThighScale.y,
      leftThighJoint.z - std::sin(leftThighRotX) * leftThighScale.y};

  modObjects_[ToIndex(ModBodyPart::RightShin)]
      ->mainPosition.transform.translate = {
      rightThighJoint.x,
      rightThighJoint.y - std::cos(rightThighRotX) * rightThighScale.y,
      rightThighJoint.z - std::sin(rightThighRotX) * rightThighScale.y};
}
