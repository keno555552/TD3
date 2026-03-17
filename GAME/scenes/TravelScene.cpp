#include "TravelScene.h"

namespace {
/*    enum class を配列添字に変換するための補助関数   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }
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
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];
  Object *leftArm = modObjects_[ToIndex(ModBodyPart::LeftArm)];
  Object *rightArm = modObjects_[ToIndex(ModBodyPart::RightArm)];
  Object *leftLeg = modObjects_[ToIndex(ModBodyPart::LeftLeg)];
  Object *rightLeg = modObjects_[ToIndex(ModBodyPart::RightLeg)];

  if (body == nullptr || head == nullptr || leftArm == nullptr ||
      rightArm == nullptr || leftLeg == nullptr || rightLeg == nullptr) {
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

  // 胴体は少し前傾・上下に潰す
  body->mainPosition.transform.rotate.z = bodyTilt_;

  body->mainPosition.transform.scale.y =
      (std::max)(0.65f, 1.0f - bodyStretch_ * 0.2f);

  // 頭は少し遅れて揺れる
  head->mainPosition.transform.rotate.x =
      (leftLegBend_ - rightLegBend_) * -0.15f;

  // 脚：曲げる
  leftLeg->mainPosition.transform.rotate.x = leftLegBend_;
  rightLeg->mainPosition.transform.rotate.x = rightLegBend_;

  // 腕：逆側に少し振る
  leftArm->mainPosition.transform.rotate.x = -rightLegBend_ * 0.6f;
  rightArm->mainPosition.transform.rotate.x = -leftLegBend_ * 0.6f;

  // 各部位オブジェクトを更新
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
  SetupPartObject(ModBodyPart::Head, "GAME/resources/modBody/head/head.obj");
  SetupPartObject(ModBodyPart::LeftArm,
                  "GAME/resources/modBody/leftArm/leftArm.obj");
  SetupPartObject(ModBodyPart::RightArm,
                  "GAME/resources/modBody/rightArm/rightArm.obj");
  SetupPartObject(ModBodyPart::LeftLeg,
                  "GAME/resources/modBody/leftLeg/leftLeg.obj");
  SetupPartObject(ModBodyPart::RightLeg,
                  "GAME/resources/modBody/rightLeg/rightLeg.obj");
  // 親子関係設定
  SetupHierarchy();

  // 初期配置設定
  SetupInitialLayout();
}

/*   指定した部位のObjectを1つ生成する   */
void TravelScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  size_t index = ToIndex(part);

  // モデル読み込み
  modModelHandles_[index] = system_->SetModelObj(path);

  // Object生成
  modObjects_[index] = new Object;
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);

  // 初期Transform設定
  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();
}

/*   部位同士の親子関係を設定する   */
void TravelScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body == nullptr) {
    return;
  }

  // BodyのmainPositionを親として使う
  ObjectPart *bodyRoot = &body->mainPosition;

  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftLeg)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightLeg)]->followObject_ = bodyRoot;
}

/*   各部位の初期配置を設定する   */
void TravelScene::SetupInitialLayout() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];
  Object *leftArm = modObjects_[ToIndex(ModBodyPart::LeftArm)];
  Object *rightArm = modObjects_[ToIndex(ModBodyPart::RightArm)];
  Object *leftLeg = modObjects_[ToIndex(ModBodyPart::LeftLeg)];
  Object *rightLeg = modObjects_[ToIndex(ModBodyPart::RightLeg)];

  // 胴体を基準位置へ配置
  body->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
  body->mainPosition.transform.rotate = {0.0f, 1.5f, 0.0f};

  // 頭を胴体の上に配置
  head->mainPosition.transform.translate = {0.0f, 1.5f, 0.0f};

  // 仮
  head->mainPosition.transform.translate = {0.0f, 1.8f, 0.0f};
  head->mainPosition.transform.scale = {1.8f, 1.8f, 1.8f};

  // 腕を左右に配置
  leftArm->mainPosition.transform.translate = {-1.25f, 0.5f, 0.0f};
  rightArm->mainPosition.transform.translate = {1.25f, 0.5f, 0.0f};

  leftLeg->mainPosition.transform.scale.y = 1.4f;
  rightLeg->mainPosition.transform.scale.y = 1.4f;

  // 脚を下に配置
  leftLeg->mainPosition.transform.translate = {-0.5f, -1.75f, 0.0f};
  rightLeg->mainPosition.transform.translate = {0.5f, -1.75f, 0.0f};
}

/*   各部位Objectの更新をまとめて行う   */
void TravelScene::UpdateModObjects() {
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
