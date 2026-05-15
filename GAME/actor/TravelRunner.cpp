#include "GAME/effect/Perfect_Particle.h"
#include "ModObjectUtil.h"
#include "TravelRunner.h"
#include "kEngine.h"
#include <algorithm>
#include <cmath>

namespace {
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }
Vector3 ScaleByRatio(const Vector3 &base, const Vector3 &ratio) {
  return {base.x * ratio.x, base.y * ratio.y, base.z * ratio.z};
}
} // namespace

TravelRunner::TravelRunner(kEngine *system) : system_(system) {
  perfectParticle_ = std::make_unique<Perfect_Particle>(system_);
}

TravelRunner::~TravelRunner() {
  for (auto &pair : allPartObjects_) {
    if (pair.second)
      delete pair.second;
  }
}

void TravelRunner::Initialize(float startX) { moveX_ = startX; }

void TravelRunner::UpdateHoldState(bool leftNowInput, bool rightNowInput,
                                   float deltaTime) {
  if (leftNowInput && isGrounded_) {
    leftHoldTime_ += deltaTime;
  } else {
    leftHoldTime_ = 0.0f;
  }

  if (rightNowInput && isGrounded_) {
    rightHoldTime_ += deltaTime;
  } else {
    rightHoldTime_ = 0.0f;
  }

  if (!leftNowInput) {
    requireReleaseAfterLandLeft_ = false;
  }
  if (!rightNowInput) {
    requireReleaseAfterLandRight_ = false;
  }
}

void TravelRunner::UpdateLegBendState(bool leftNowInput, bool rightNowInput) {
  float leftTargetLegAngle = leftNowInput ? legKickAngle_ : legRecoverAngle_;
  float rightTargetLegAngle = rightNowInput ? legKickAngle_ : legRecoverAngle_;

  float leftFollow = legFollowPower_;
  float rightFollow = legFollowPower_;

  float leftMaxSpeed = legMaxSpeed_;
  float rightMaxSpeed = legMaxSpeed_;

  float tiltDiff = bodyTilt_ - idealRunTilt_;
  float forwardLean = (std::max)(0.0f, -tiltDiff);

  float returnPenalty = 1.0f - forwardLean * 0.65f;
  returnPenalty = std::clamp(returnPenalty, 0.35f, 1.0f);

  if (!leftNowInput) {
    leftFollow *= leftLegReturnScale_ * leftLegReturnScale_ *
                  leftLegReturnScale_ * 0.25f * returnPenalty;
    leftMaxSpeed *= leftLegReturnScale_ * leftLegReturnScale_ *
                    leftLegReturnScale_ * returnPenalty;
  }
  if (!rightNowInput) {
    rightFollow *= rightLegReturnScale_ * rightLegReturnScale_ *
                   rightLegReturnScale_ * 0.25f * returnPenalty;
    rightMaxSpeed *= rightLegReturnScale_ * rightLegReturnScale_ *
                     rightLegReturnScale_ * returnPenalty;
  }

  leftLegBendSpeed_ += (leftTargetLegAngle - leftLegBend_) * leftFollow;
  rightLegBendSpeed_ += (rightTargetLegAngle - rightLegBend_) * rightFollow;

  leftLegBendSpeed_ =
      std::clamp(leftLegBendSpeed_, -leftMaxSpeed, leftMaxSpeed);
  rightLegBendSpeed_ =
      std::clamp(rightLegBendSpeed_, -rightMaxSpeed, rightMaxSpeed);

  leftLegBendSpeed_ *= jointDamping_;
  rightLegBendSpeed_ *= jointDamping_;
  bodyStretchSpeed_ *= jointDamping_;

  leftLegBend_ += leftLegBendSpeed_;
  rightLegBend_ += rightLegBendSpeed_;
  bodyStretch_ += bodyStretchSpeed_;

  leftLegBend_ = std::clamp(leftLegBend_, -0.8f, 1.0f);
  rightLegBend_ = std::clamp(rightLegBend_, -0.8f, 1.0f);

  bodyStretch_ = std::clamp(bodyStretch_, -0.5f, 1.0f);
}

void TravelRunner::UpdateModObjects() {
  for (auto &pair : allPartObjects_) {
    if (pair.second)
      pair.second->Update(nullptr);
  }
  if (shadow_ != nullptr) {
    shadow_->mainPosition.transform.translate.x = laneX_;
    shadow_->mainPosition.transform.translate.y = groundY_ + 0.01f;
    shadow_->mainPosition.transform.translate.z = moveX_;
    shadow_->mainPosition.transform.rotate = {1.5f, -1.57f, 0.0f};

    LowestBodyPart lowestPart = LowestBodyPart::None;
    float lowestBodyLocalY = GetLowestVisualBodyY(&lowestPart);
    float lowestBodyWorldY = moveY_ + visualLiftY_ + lowestBodyLocalY;

    float jumpHeight = lowestBodyWorldY - groundY_;
    if (jumpHeight < 0.0f) {
      jumpHeight = 0.0f;
    }

    float scale = 1.2f - jumpHeight * 0.35f;
    scale = std::clamp(scale, 0.1f, 30.0f);

    shadow_->mainPosition.transform.scale = {scale, scale, 1.0f};
    shadow_->Update(nullptr);
  }
}

void TravelRunner::DrawModObjects(Camera *camera) {
  for (auto &pair : allPartObjects_) {
    if (pair.second)
      pair.second->Draw();
  }
}

void TravelRunner::UpdateParticle(Camera *camera) {
  if (perfectParticle_) {
    perfectParticle_->Update(camera);
  }
}

void TravelRunner::DrawParticle() {
  if (perfectParticle_) {
    perfectParticle_->Draw();
  }
}

void TravelRunner::ClearParticle() {
  if (perfectParticle_) {
    perfectParticle_->ClearAll();
  }
}

void TravelRunner::LoadCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  // ModScene から引き継いだ残り時間を復元する
  /*timeLimit_ = customizeData_->timeLimit_;
  travelTimeLimit_ = timeLimit_;
  isTimeUp_ = customizeData_->isTimeUp_;*/

  controlPointSnapshots_ = customizeData_->controlPointSnapshots;

  const auto &cp = customizeData_->controlPoints;

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartType == ModBodyPart::RightUpperArm) {
    }
  }
}

void TravelRunner::ApplyCustomizeToMovementParam() {

  if (customizeData_ == nullptr) {
    return;
  }

  // 無改造基準
  const float baseLegLength = 2.0f;
  const float baseLegThicknessScale = 0.95f;
  const float baseLegSlimness =
      baseLegLength / (baseLegThicknessScale + 0.001f);

  const float baseHeadSizeScale = 1.684f;
  const float baseTorsoScaleAvg = 1.899f;
  const float baseAsymmetry = 0.0f;
  const float baseCenterOfMassY = 0.150f;

  //==============================
  // 各部位のパラメータ取得
  // 長さは scale.y で扱う
  //==============================
  auto GetPartParam = [&](ModBodyPart partType) -> ModBodyPartParam {
    int partId = -1;
    if (GetFirstPartTypePartId(partType, partId)) {
      for (const auto &inst : customizeData_->partInstances) {
        if (inst.partId == partId)
          return inst.param;
      }
    }
    ModBodyPartParam empty{};
    empty.scale = {1.0f, 1.0f, 1.0f};
    empty.length = 1.0f;
    return empty;
  };
  const auto &chest = GetPartParam(ModBodyPart::ChestBody);
  const auto &stomach = GetPartParam(ModBodyPart::StomachBody);
  const auto &neck = GetPartParam(ModBodyPart::Neck);
  const auto &head = GetPartParam(ModBodyPart::Head);

  const auto &leftUpperArm = GetPartParam(ModBodyPart::LeftUpperArm);
  const auto &rightUpperArm = GetPartParam(ModBodyPart::RightUpperArm);
  const auto &leftForeArm = GetPartParam(ModBodyPart::LeftForeArm);
  const auto &rightForeArm = GetPartParam(ModBodyPart::RightForeArm);

  const auto &leftThigh = GetPartParam(ModBodyPart::LeftThigh);
  const auto &rightThigh = GetPartParam(ModBodyPart::RightThigh);
  const auto &leftShin = GetPartParam(ModBodyPart::LeftShin);
  const auto &rightShin = GetPartParam(ModBodyPart::RightShin);

  auto AvgXZ = [](const ModBodyPartParam &p) -> float {
    return (p.scale.x + p.scale.z) * 0.5f;
  };

  //==============================
  // 長さ（scale.y をそのまま使う）
  //==============================
  const float leftLegLength = leftThigh.scale.y + leftShin.scale.y;
  const float rightLegLength = rightThigh.scale.y + rightShin.scale.y;
  const float avgLegLength = (leftLegLength + rightLegLength) * 0.5f;

  const float neckLengthScale = neck.scale.y;

  //==============================
  // 太さ
  //==============================
  const float baseRadius = 0.1f;

  const float leftThighStartR = GetSnapshotRadius(ModBodyPart::LeftThigh, 1);
  const float leftThighBendR = GetSnapshotRadius(ModBodyPart::LeftThigh, 2);
  const float leftThighEndR = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);

  const float rightThighStartR = GetSnapshotRadius(ModBodyPart::RightThigh, 1);
  const float rightThighBendR = GetSnapshotRadius(ModBodyPart::RightThigh, 2);
  const float rightThighEndR = GetSnapshotRadius(ModBodyPart::RightThigh, 3);

  // 太さは見た目と同じく snapshot 半径ベースで取る
  const float leftThighThickness =
      (std::max)(leftThighStartR, leftThighBendR) / baseRadius;
  const float leftShinThickness =
      (std::max)(leftThighBendR, leftThighEndR) / baseRadius;

  const float rightThighThickness =
      (std::max)(rightThighStartR, rightThighBendR) / baseRadius;
  const float rightShinThickness =
      (std::max)(rightThighBendR, rightThighEndR) / baseRadius;

  const float leftLegThickness =
      (leftThighThickness + leftShinThickness) * 0.5f;
  const float rightLegThickness =
      (rightThighThickness + rightShinThickness) * 0.5f;

  const float legThicknessScale = (leftLegThickness + rightLegThickness) * 0.5f;

  const float leftArmThickness =
      (AvgXZ(leftUpperArm) + AvgXZ(leftForeArm)) * 0.5f;
  const float rightArmThickness =
      (AvgXZ(rightUpperArm) + AvgXZ(rightForeArm)) * 0.5f;
  const float armThicknessScale = (leftArmThickness + rightArmThickness) * 0.5f;

  const float neckThicknessScale = AvgXZ(neck);

  //==============================
  // 頭サイズは snapshot + control point ベースで取る
  //==============================
  const float upperNeckR = GetSnapshotRadius(ModBodyPart::Head, 8);
  const float headCenterR = GetSnapshotRadius(ModBodyPart::Head, 9);

  const float headThicknessScale =
      (std::max)(upperNeckR, headCenterR) / baseRadius;

  const Vector3 &upperNeckPos = customizeData_->controlPoints.upperNeckPos;
  const Vector3 &headCenterPos = customizeData_->controlPoints.headCenterPos;

  Vector3 headVec = {headCenterPos.x - upperNeckPos.x,
                     headCenterPos.y - upperNeckPos.y,
                     headCenterPos.z - upperNeckPos.z};

  const float currentHeadLength = Length(headVec);
  const float baseHeadLength = 0.55f;

  float headLengthScale = 1.0f;
  if (currentHeadLength > 0.0001f) {
    headLengthScale = currentHeadLength / baseHeadLength;
  }

  // 太さ寄りで平均
  const float headSizeScale =
      (headThicknessScale * 2.0f + headLengthScale) / 3.0f;

  headSizeScale_ = headSizeScale;

  //==============================
  // 胴体サイズ
  // 旧: 太さだけ
  // 新: 頭と同じ考え方で「太さ + 長さ」
  //==============================
  const float chestR = GetControlPointRadius(ModControlPointRole::Chest);
  const float bellyR = GetControlPointRadius(ModControlPointRole::Belly);
  const float waistR = GetControlPointRadius(ModControlPointRole::Waist);

  const float chestThicknessScale = (std::max)(chestR, bellyR) / baseRadius;
  const float stomachThicknessScale = (std::max)(bellyR, waistR) / baseRadius;

  const Vector3 &chestPos = customizeData_->controlPoints.chestPos;
  const Vector3 &bellyPos = customizeData_->controlPoints.bellyPos;
  const Vector3 &waistPos = customizeData_->controlPoints.waistPos;

  Vector3 chestToBelly = {bellyPos.x - chestPos.x, bellyPos.y - chestPos.y,
                          bellyPos.z - chestPos.z};
  Vector3 bellyToWaist = {waistPos.x - bellyPos.x, waistPos.y - bellyPos.y,
                          waistPos.z - bellyPos.z};

  const float chestLength = Length(chestToBelly);
  const float stomachLength = Length(bellyToWaist);

  // 無改造基準は今の見た目に合わせた仮値
  const float baseChestLength = 0.45f;
  const float baseStomachLength = 0.45f;

  float chestLengthScale = 1.0f;
  if (chestLength > 0.0001f) {
    chestLengthScale = chestLength / baseChestLength;
  }

  float stomachLengthScale = 1.0f;
  if (stomachLength > 0.0001f) {
    stomachLengthScale = stomachLength / baseStomachLength;
  }

  // 頭と同じく「太さ寄り」で平均
  const float chestScale =
      (chestThicknessScale * 2.0f + chestLengthScale) / 3.0f;
  const float stomachScale =
      (stomachThicknessScale * 2.0f + stomachLengthScale) / 3.0f;

  const float torsoScaleAvg = (chestScale + stomachScale) * 0.5f;

  torsoSizeScale_ = torsoScaleAvg;

  torsoStabilityScale_ = std::clamp(
      1.0f + (torsoScaleAvg - baseTorsoScaleAvg) * 3.00f, 0.45f, 4.20f);

  torsoTiltResistance_ = std::clamp(
      1.0f - (torsoScaleAvg - baseTorsoScaleAvg) * 1.80f, 0.10f, 1.15f);

  //==============================
  // 左右差
  //==============================
  const float legDiff = std::abs(leftLegLength - rightLegLength);

  //==============================
  // 細長さ
  //==============================
  const float legSlimness = avgLegLength / (legThicknessScale + 0.001f);

  //==============================
  // ベース
  //==============================
  tuning_.runPower = 1.0f;
  tuning_.maxSpeed = 5.0f;
  tuning_.stability = 1.0f;
  tuning_.lift = 3.0f;
  tuning_.turnResponse = 1.0f;

  //==============================
  // 前進力
  //==============================
  tuning_.runPower += (avgLegLength - baseLegLength) * 2.2f;
  tuning_.runPower -= (legThicknessScale - baseLegThicknessScale) * 1.2f;
  tuning_.runPower -= legDiff * 1.8f;

  //==============================
  // 最高速
  //==============================
  tuning_.maxSpeed += (avgLegLength - baseLegLength) * 2.7f;
  tuning_.maxSpeed -= (legThicknessScale - baseLegThicknessScale) * 0.9f;
  tuning_.maxSpeed -= (headSizeScale - baseHeadSizeScale) * 0.5f;
  tuning_.maxSpeed -= legDiff * 1.6f;

  //==============================
  // 安定性
  //==============================
  tuning_.stability += (legThicknessScale - baseLegThicknessScale) * 1.5f;
  tuning_.stability += (armThicknessScale - 1.0f) * 0.5f;
  tuning_.stability += (neckThicknessScale - 1.0f) * 0.7f;

  tuning_.stability -= (legSlimness - baseLegSlimness) * 1.5f;
  tuning_.stability -= (neckLengthScale - 1.0f) * 1.2f;
  tuning_.stability -= (headSizeScale - baseHeadSizeScale) * 1.0f;
  tuning_.stability -= legDiff * 1.8f;
  tuning_.stability -= (features_.asymmetry - baseAsymmetry) * 0.35f;

  //==============================
  // 上方向
  //==============================
  tuning_.lift += (avgLegLength - baseLegLength) * 2.3f;
  tuning_.lift -= (legThicknessScale - baseLegThicknessScale) * 0.7f;
  tuning_.lift -= (headSizeScale - baseHeadSizeScale) * 0.3f;

  float comOffset =
      std::clamp(features_.centerOfMassY - baseCenterOfMassY, -3.0f, 3.0f);
  tuning_.lift += comOffset * 0.2f;

  //==============================
  // 傾きやすさ
  //==============================
  tuning_.turnResponse += (legSlimness - baseLegSlimness) * 1.2f;
  tuning_.turnResponse += legDiff * 2.0f;
  tuning_.turnResponse += (neckLengthScale - 1.0f) * 0.8f;
  tuning_.turnResponse += (features_.asymmetry - baseAsymmetry) * 0.5f;

  //==============================
  // 追加パーツの影響
  //==============================
  float extraHeadCount = (std::max)(0.0f, features_.headCount - 1.0f);
  float extraArmCount = (std::max)(0.0f, features_.armCount - 2.0f);
  float extraLegCount = (std::max)(0.0f, features_.legCount - 2.0f);

  tuning_.stability -= extraHeadCount * 0.8f;
  tuning_.turnResponse += extraHeadCount * 0.8f;
  tuning_.lift += extraHeadCount * 0.2f;

  tuning_.stability -= extraArmCount * 0.15f;
  tuning_.runPower += extraArmCount * 0.1f;

  tuning_.stability += extraLegCount * 1.5f;
  tuning_.runPower += extraLegCount * 0.6f;
  tuning_.maxSpeed -= extraLegCount * 0.2f;
  tuning_.turnResponse -= extraLegCount * 0.5f;

  //==============================
  // クランプ
  //==============================
  tuning_.runPower = std::clamp(tuning_.runPower, 0.1f, 4.0f);
  tuning_.maxSpeed = std::clamp(tuning_.maxSpeed, 0.2f, 4.5f);
  tuning_.stability = std::clamp(tuning_.stability, 0.1f, 3.5f);
  tuning_.lift = std::clamp(tuning_.lift, 0.2f, 3.5f);
  tuning_.turnResponse = std::clamp(tuning_.turnResponse, 0.2f, 4.0f);

  leftLegReturnScale_ = std::clamp(
      1.0f - (leftLegThickness - baseLegThicknessScale) * 0.20f, 0.55f, 1.0f);

  rightLegReturnScale_ = std::clamp(
      1.0f - (rightLegThickness - baseLegThicknessScale) * 0.20f, 0.55f, 1.0f);

  // 安定してる体ほどタイミング判定を広く
  timingWindowScale_ = std::clamp(0.8f + tuning_.stability * 0.25f -
                                      tuning_.turnResponse * 0.08f,
                                  0.55f, 1.25f);

  // 安定してる体ほど起き上がりやすい
  recoveryAssist_ = std::clamp(0.7f + tuning_.stability * 0.25f, 0.6f, 1.4f);
}

float TravelRunner::ComputeLegHeightOffset() const {
  const ModControlPointData *cp = GetControlPoints();
  if (cp == nullptr) {
    return 0.0f;
  }

  auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
  };

  const float leftThighLength = Length(Sub(cp->leftKneePos, cp->leftHipPos));
  const float leftShinLength = Length(Sub(cp->leftAnklePos, cp->leftKneePos));

  const float rightThighLength = Length(Sub(cp->rightKneePos, cp->rightHipPos));
  const float rightShinLength =
      Length(Sub(cp->rightAnklePos, cp->rightKneePos));

  const float leftLegLength = leftThighLength + leftShinLength;
  const float rightLegLength = rightThighLength + rightShinLength;
  const float avgLegLength = (leftLegLength + rightLegLength) * 0.5f;

  // 無改造基準
  const float baseThighLength = 1.25f;
  const float baseShinLength = 1.0f;
  const float baseLegLength = baseThighLength + baseShinLength;

  return avgLegLength - baseLegLength;
}

void TravelRunner::SavePreviousFrameState() {
  leftLegPrevBend_ = leftLegBend_;
  rightLegPrevBend_ = rightLegBend_;

  leftLegPrevBendSpeed_ = leftLegBendSpeed_;
  rightLegPrevBendSpeed_ = rightLegBendSpeed_;
}

void TravelRunner::UpdateMovementState(bool leftNowInput, bool rightNowInput) {

  bool bothInput = leftNowInput && rightNowInput;

  kickFeedbackType_ = KickFeedbackType::None;

  float stability = useCustomizeMove_ ? tuning_.stability : 1.0f;
  float turnResponse = useCustomizeMove_ ? tuning_.turnResponse : 1.0f;

  const float recoverStartTilt = 0.42f;
  const float recoverTargetTilt = -0.12f;
  const float heavyFallTilt = 0.65f;

  isRecoveringFromTilt_ = (std::abs(bodyTilt_) > recoverStartTilt);

  //==============================
  // 姿勢：通常時は中立へ戻すだけ
  // 蹴った瞬間にだけ bodyTiltVelocity_ を入れる
  //==============================
  const float neutralTilt = -0.03f;

  float headRecoveryPenalty =
      std::clamp(1.0f - (headSizeScale_ - 1.0f) * 0.55f, 0.25f, 1.0f);

  // 常時戻しを弱くして、姿勢を少し引きずるようにする
  float neutralReturnPower =
      0.020f * stability * headRecoveryPenalty * torsoStabilityScale_;

  if (bothInput) {
    neutralReturnPower *= 0.80f;
  }

  // 空中ではさらに戻りにくくする
  if (!isGrounded_) {
    neutralReturnPower *= 0.35f;
  }

  bodyTiltVelocity_ += (neutralTilt - bodyTilt_) * neutralReturnPower;

  //==============================
  // 姿勢の良し悪し
  //==============================
  float postureError = std::abs(bodyTilt_ - idealRunTilt_);
  float badPosture = std::clamp(postureError / postureTolerance_, 0.0f, 1.0f);
  float forwardRate = 1.0f - badPosture;

  const ModControlPointData *cp = GetControlPoints();

  float lowestLegBottomLocalY = 0.0f;
  const float groundEpsilon = 0.01f;
  // bool isLeftFootGrounded = isGrounded_;
  // bool isRightFootGrounded = isGrounded_;

  if (cp != nullptr && customizeData_ != nullptr) {
    Vector3 leftHipAnchorLocal = {-0.5f, -1.25f, 0.0f};
    Vector3 rightHipAnchorLocal = {0.5f, -1.25f, 0.0f};

    Vector3 leftLegRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 leftLegBendLocal = {0.0f, -0.70f, 0.0f};
    Vector3 leftLegEndLocal = {0.0f, -1.40f, 0.0f};

    Vector3 rightLegRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 rightLegBendLocal = {0.0f, -0.70f, 0.0f};
    Vector3 rightLegEndLocal = {0.0f, -1.40f, 0.0f};

    int torsoAnchorOwnerId = -1;
    int leftThighOwnerId = -1;
    int rightThighOwnerId = -1;

    for (const auto &instance : customizeData_->partInstances) {
      if (torsoAnchorOwnerId < 0 &&
          instance.partType == ModBodyPart::ChestBody) {
        torsoAnchorOwnerId = instance.partId;
      }
      if (leftThighOwnerId < 0 && instance.partType == ModBodyPart::LeftThigh) {
        leftThighOwnerId = instance.partId;
      }
      if (rightThighOwnerId < 0 &&
          instance.partType == ModBodyPart::RightThigh) {
        rightThighOwnerId = instance.partId;
      }
    }

    for (const auto &snap : customizeData_->controlPointSnapshots) {
      if (snap.ownerPartId == torsoAnchorOwnerId) {
        if (snap.role == ModControlPointRole::LeftHip) {
          leftHipAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::RightHip) {
          rightHipAnchorLocal = snap.localPosition;
        }
      }

      if (snap.ownerPartId == leftThighOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          leftLegRootLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::Bend) {
          leftLegBendLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::End) {
          leftLegEndLocal = snap.localPosition;
        }
      }

      if (snap.ownerPartId == rightThighOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          rightLegRootLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::Bend) {
          rightLegBendLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::End) {
          rightLegEndLocal = snap.localPosition;
        }
      }
    }

    const float leftAnkleRadius = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);
    const float rightAnkleRadius =
        GetSnapshotRadius(ModBodyPart::RightThigh, 3);

    // 実際の見た目配置に合わせた ankle 下端
    auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
      return {a.x - b.x, a.y - b.y, a.z - b.z};
    };

    auto NormalizeSafe = [](const Vector3 &v) -> Vector3 {
      float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
      if (len < 0.0001f) {
        return {0.0f, -1.0f, 0.0f};
      }
      return {v.x / len, v.y / len, v.z / len};
    };

    auto BuildAnimatedChildRoot = [](const Vector3 &root, float angleZ,
                                     float angleX, float length) -> Vector3 {
      return {root.x + std::sin(angleZ) * std::cos(angleX) * length,
              root.y - std::cos(angleZ) * std::cos(angleX) * length,
              root.z - std::sin(angleX) * length};
    };

    const float thighSwingScale = 0.70f;

    //==============================
    // 左脚：アニメ後の足先位置
    //==============================
    Vector3 leftThighVec = Sub(leftLegBendLocal, leftLegRootLocal);
    float leftThighLength = Length(leftThighVec);
    if (leftThighLength < 0.0001f) {
      leftThighVec = {0.0f, -1.0f, 0.0f};
      leftThighLength = 0.0001f;
    }

    float leftThighScaleY = 1.0f;
    float leftShinScaleY = 1.0f;
    float rightThighScaleY = 1.0f;
    float rightShinScaleY = 1.0f;
    if (customizeData_) {
      for (const auto &inst : customizeData_->partInstances) {
        if (inst.partType == ModBodyPart::LeftThigh)
          leftThighScaleY = inst.param.scale.y * inst.param.length;
        if (inst.partType == ModBodyPart::LeftShin)
          leftShinScaleY = inst.param.scale.y * inst.param.length;
        if (inst.partType == ModBodyPart::RightThigh)
          rightThighScaleY = inst.param.scale.y * inst.param.length;
        if (inst.partType == ModBodyPart::RightShin)
          rightShinScaleY = inst.param.scale.y * inst.param.length;
      }
    }

    leftThighLength *= leftThighScaleY;

    Vector3 leftThighDir = NormalizeSafe(leftThighVec);

    float leftThighAngleZ = std::atan2(leftThighDir.x, -leftThighDir.y);
    float leftThighBaseX = -std::asin(std::clamp(leftThighDir.z, -1.0f, 1.0f));
    float leftThighAnimX = -leftLegBend_ * thighSwingScale;
    float leftThighAngleX = leftThighBaseX + leftThighAnimX;

    Vector3 leftShinVec = Sub(leftLegEndLocal, leftLegBendLocal);
    float leftShinLength = Length(leftShinVec);
    if (leftShinLength < 0.0001f) {
      leftShinVec = {0.0f, -1.0f, 0.0f};
      leftShinLength = 0.0001f;
    }
    leftShinLength *= leftShinScaleY;
    Vector3 leftShinDir = NormalizeSafe(leftShinVec);

    float leftShinAngleZ = std::atan2(leftShinDir.x, -leftShinDir.y);
    float leftShinBaseX = -std::asin(std::clamp(leftShinDir.z, -1.0f, 1.0f));

    float leftThighSwing = -leftLegBend_ * thighSwingScale;
    float leftKneeFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                        (legRecoverAngle_ - legKickAngle_),
                                    0.0f, 1.0f);
    float leftShinAnimX = leftThighSwing * 0.35f + leftKneeFold * 0.6f;
    float leftShinAngleX = leftShinBaseX + leftShinAnimX;

    Vector3 leftThighRoot = leftHipAnchorLocal;
    Vector3 leftShinRoot = BuildAnimatedChildRoot(
        leftThighRoot, leftThighAngleZ, leftThighAngleX, leftThighLength);
    Vector3 leftFootPos = BuildAnimatedChildRoot(
        leftShinRoot, leftShinAngleZ, leftShinAngleX, leftShinLength);

    const float leftFootBottomLocalY = leftFootPos.y - leftAnkleRadius;

    //==============================
    // 右脚：アニメ後の足先位置
    //==============================
    Vector3 rightThighVec = Sub(rightLegBendLocal, rightLegRootLocal);
    float rightThighLength = Length(rightThighVec);
    if (rightThighLength < 0.0001f) {
      rightThighVec = {0.0f, -1.0f, 0.0f};
      rightThighLength = 0.0001f;
    }
    rightThighLength *= rightThighScaleY;
    Vector3 rightThighDir = NormalizeSafe(rightThighVec);

    float rightThighAngleZ = std::atan2(rightThighDir.x, -rightThighDir.y);
    float rightThighBaseX =
        -std::asin(std::clamp(rightThighDir.z, -1.0f, 1.0f));
    float rightThighAnimX = -rightLegBend_ * thighSwingScale;
    float rightThighAngleX = rightThighBaseX + rightThighAnimX;

    Vector3 rightShinVec = Sub(rightLegEndLocal, rightLegBendLocal);
    float rightShinLength = Length(rightShinVec);
    if (rightShinLength < 0.0001f) {
      rightShinVec = {0.0f, -1.0f, 0.0f};
      rightShinLength = 0.0001f;
    }
    rightShinLength *= rightShinScaleY;
    Vector3 rightShinDir = NormalizeSafe(rightShinVec);

    float rightShinAngleZ = std::atan2(rightShinDir.x, -rightShinDir.y);
    float rightShinBaseX = -std::asin(std::clamp(rightShinDir.z, -1.0f, 1.0f));

    float rightThighSwing = -rightLegBend_ * thighSwingScale;
    float rightKneeFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                         (legRecoverAngle_ - legKickAngle_),
                                     0.0f, 1.0f);
    float rightShinAnimX = rightThighSwing * 0.35f + rightKneeFold * 0.6f;
    float rightShinAngleX = rightShinBaseX + rightShinAnimX;

    Vector3 rightThighRoot = rightHipAnchorLocal;
    Vector3 rightShinRoot = BuildAnimatedChildRoot(
        rightThighRoot, rightThighAngleZ, rightThighAngleX, rightThighLength);
    Vector3 rightFootPos = BuildAnimatedChildRoot(
        rightShinRoot, rightShinAngleZ, rightShinAngleX, rightShinLength);

    const float rightFootBottomLocalY = rightFootPos.y - rightAnkleRadius;

    float predictedFirstLegsY =
        (std::min)(leftFootBottomLocalY, rightFootBottomLocalY);

    float visualLowestY = GetLowestVisualBodyY(nullptr);
    if (visualLowestY < 900000.0f) {
      lowestLegBottomLocalY = (std::min)(predictedFirstLegsY, visualLowestY);
    } else {
      lowestLegBottomLocalY = predictedFirstLegsY;
    }

    const float groundEpsilon = 0.01f;

    // const float leftContactMoveY = groundY_ - leftFootBottomLocalY;
    // const float rightContactMoveY = groundY_ - rightFootBottomLocalY;

    // isLeftFootGrounded = (moveY_ <= leftContactMoveY + groundEpsilon);
    // isRightFootGrounded = (moveY_ <= rightContactMoveY + groundEpsilon);
  }

  //==============================
  // 入力保持
  //==============================
  bool leftHoldEnough = (leftHoldTime_ >= minHoldTimeToKick_);
  bool rightHoldEnough = (rightHoldTime_ >= minHoldTimeToKick_);

  float leftPushCandidate = 0.0f;
  float rightPushCandidate = 0.0f;

  bool useLeftPush = false;
  bool useRightPush = false;
  float adoptedDriveUse = 0.0f;

  float singleStepBasePush = 0.12f;
  float singleStepJumpScale = 0.7f;

  //==============================
  // 左脚候補
  //==============================
  if (leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      !requireReleaseAfterLandLeft_) {

    float sequenceBonus = 1.0f;
    if (lastKickSide_ == 1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == -1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = 0.0f;
    if (leftDriveAccum_ >= minDriveToKick_ && leftHoldEnough) {
      driveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);
    }

    float drivePush = driveUse * drivePushScale_ * pushPower_ * groundAssist_ *
                      groundedKickFactor_ * 2.0f * sequenceBonus;

    float basePush = singleStepBasePush * pushPower_ * sequenceBonus;

    leftPushCandidate = (std::max)(basePush, drivePush);
  }

  //==============================
  // 右脚候補
  //==============================
  if (rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      !requireReleaseAfterLandRight_) {

    float sequenceBonus = 1.0f;
    if (lastKickSide_ == -1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == 1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = 0.0f;
    if (rightDriveAccum_ >= minDriveToKick_ && rightHoldEnough) {
      driveUse = (std::min)(rightDriveAccum_, driveUsePerFrame_);
    }

    float drivePush = driveUse * drivePushScale_ * pushPower_ * groundAssist_ *
                      groundedKickFactor_ * 2.0f * sequenceBonus;

    float basePush = singleStepBasePush * pushPower_ * sequenceBonus;

    rightPushCandidate = (std::max)(basePush, drivePush);
  }

  //==============================
  // 採用する蹴りを決定
  //==============================
  float totalPush = 0.0f;

  if (leftPushCandidate >= rightPushCandidate && leftPushCandidate > 0.0f) {
    totalPush = leftPushCandidate;
    useLeftPush = true;
    adoptedDriveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);
    lastKickSide_ = -1;

    leftDriveAccum_ -= adoptedDriveUse;
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_);

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

  //==============================
  // 脚の戻り具合で蹴り威力を補正
  //==============================
  float kickLegBendNow = 0.0f;

  if (useLeftPush) {
    kickLegBendNow = leftLegBend_;
  } else if (useRightPush) {
    kickLegBendNow = rightLegBend_;
  }

  // 戻り具合（0〜1）
  float kickReadyRatio = std::clamp((kickLegBendNow - legKickAngle_) /
                                        (legRecoverAngle_ - legKickAngle_),
                                    0.0f, 1.0f);

  // 少ししか戻していない蹴りはほぼ無意味にする
  const float minKickReady = 0.35f;

  float normalizedKickReady = std::clamp(
      (kickReadyRatio - minKickReady) / (1.0f - minKickReady), 0.0f, 1.0f);

  // 二乗で弱い蹴りをさらに弱くする
  float kickReadyPower = normalizedKickReady * normalizedKickReady;

  // 姿勢が悪いほど蹴りの力が地面に乗らない
  float postureKickScale = 1.0f - badPosture * 0.55f;
  postureKickScale = std::clamp(postureKickScale, 0.35f, 1.0f);

  // 前傾しすぎは特に蹴りづらくする
  float overForwardPenalty = 1.0f;
  if (bodyTilt_ < idealRunTilt_) {
    float forwardOver =
        std::clamp((idealRunTilt_ - bodyTilt_) / 0.22f, 0.0f, 1.0f);
    overForwardPenalty = 1.0f - forwardOver * 0.35f;
  }

  totalPush *= kickReadyPower;
  // totalPush *= postureKickScale;
  // totalPush *= overForwardPenalty;

  bool startStepTrigger = isGrounded_ && landTimer_ > 0.30f && totalPush > 0.0f;

  //==============================
  // 着地タイミングで push を補正
  //==============================
  if (totalPush > 0.0f) {

    const float bestTimingEnd = 0.08f;
    const float lateTimingEnd = 0.45f;

    const float perfectTimingEnd = bestTimingEnd * 0.7f;

    float tiltImpulse = 0.0f;

    if (!startStepTrigger && isGrounded_) {
      if (landTimer_ <= perfectTimingEnd) {
        kickFeedbackType_ = KickFeedbackType::Perfect;
        kickFeedbackTimer_ = 0.18f;
        perfectStreak_++;

        perfectParticle_->Spawn({0.0f, 0.0f, moveX_}, KickEffectType::Perfect);
        Logger::Log("KICK : PERFECT");

      } else if (landTimer_ <= bestTimingEnd) {
        kickFeedbackType_ = KickFeedbackType::Good;
        kickFeedbackTimer_ = 0.12f;
        perfectStreak_ = 0;

        perfectParticle_->Spawn({0.0f, 0.0f, moveX_}, KickEffectType::Good);
        Logger::Log("KICK : GOOD");

      } else {
        kickFeedbackType_ = KickFeedbackType::Bad;

        perfectStreak_ = 0;

        perfectParticle_->Spawn({0.0f, 0.0f, moveX_}, KickEffectType::Bad);
        Logger::Log("KICK : BAD");
      }
    }

    float timingBonus = 1.0f;

    if (kickFeedbackType_ == KickFeedbackType::Perfect) {
      timingBonus = 1.60f;
    } else if (kickFeedbackType_ == KickFeedbackType::Good) {
      timingBonus = 1.00f;
    } else if (kickFeedbackType_ == KickFeedbackType::Bad) {
      timingBonus = 0.85f;
    }

    totalPush *= timingBonus;

    if (startStepTrigger) {
      tiltImpulse = 0.0f;

    } else if (!isGrounded_) {
      // 空中入力：ほぼ無効
      // totalPush *= 0.20f;
      tiltImpulse = 0.0f;

    } else if (landTimer_ <= bestTimingEnd) {
      // ベスト：蹴った瞬間に少し前傾へ入る
      tiltImpulse = -0.020f;

    } else if (landTimer_ <= lateTimingEnd) {
      // 遅い：少しずつ後傾へ
      float lateRatio =
          (landTimer_ - bestTimingEnd) / (lateTimingEnd - bestTimingEnd);
      lateRatio = std::clamp(lateRatio, 0.0f, 1.0f);

      // totalPush *= (0.90f - lateRatio * 0.20f);
      tiltImpulse = 0.010f + lateRatio * 0.020f;

    } else {
      // 遅すぎる：かなり後傾、前進も弱い
      // totalPush *= 0.20f;
      tiltImpulse = 0.035f;
    }

    float headHeavyFactor =
        std::clamp(1.0f + (headSizeScale_ - 1.0f) * 3.0f, 1.0f, 5.0f);

    bodyTiltVelocity_ +=
        tiltImpulse * turnResponse * headHeavyFactor * torsoTiltResistance_;

    const Vector3 lt = GetStandardPart(ModBodyPart::LeftThigh)
                           ->objectParts_[0]
                           .transform.scale;
    const Vector3 rt = GetStandardPart(ModBodyPart::RightThigh)
                           ->objectParts_[0]
                           .transform.scale;
    const Vector3 ls =
        GetStandardPart(ModBodyPart::LeftShin)->objectParts_[0].transform.scale;
    const Vector3 rs = GetStandardPart(ModBodyPart::RightShin)
                           ->objectParts_[0]
                           .transform.scale;

    float avgLegScaleY = (lt.y + rt.y + ls.y + rs.y) * 0.25f;
    float legLengthScale =
        std::clamp(1.0f + (avgLegScaleY - 1.0f) * 0.75f, 0.45f, 2.0f);

    float kickLegBend = 0.0f;
    if (useLeftPush) {
      kickLegBend = leftLegBend_;
    } else if (useRightPush) {
      kickLegBend = rightLegBend_;
    }

    float kickLegForwardness =
        1.0f - std::clamp((kickLegBend - legKickAngle_) /
                              (legRecoverAngle_ - legKickAngle_),
                          0.0f, 1.0f);

    float kickEfficiency = 0.75f + kickLegForwardness * 0.75f;
    float groundBoost = isGrounded_ ? 1.2f : 0.6f;

    float runPower = useCustomizeMove_ ? tuning_.runPower : 1.0f;
    float lift = useCustomizeMove_ ? tuning_.lift : 1.0f;

    //==============================
    // まず総量だけ決める
    //==============================
    float pushMagnitude = totalPush *
                          (0.55f + runPower * 1.10f + lift * 0.75f) *
                          legLengthScale * kickEfficiency * groundBoost;

    if (bothInput) {
      pushMagnitude *= 0.85f;
    } else {
      pushMagnitude *= 1.10f;
    }

    //==============================
    // 姿勢から方向だけ決める
    //==============================
    float tiltDiff = bodyTilt_ - idealRunTilt_;

    float forwardRatio = 0.5f - tiltDiff * 0.25f;
    float upwardRatio = 0.5f + tiltDiff * 0.25f;

    // 後傾側だけ追加で前進を削る
    if (tiltDiff > 0.0f) {
      float backwardPenalty = std::clamp(tiltDiff / 0.22f, 0.0f, 1.0f);
      forwardRatio *= (1.0f - backwardPenalty * 0.55f);
    }

    forwardRatio = std::clamp(forwardRatio, 0.15f, 1.50f);
    upwardRatio = std::clamp(upwardRatio, 0.15f, 1.50f);

    float dirLen =
        std::sqrt(forwardRatio * forwardRatio + upwardRatio * upwardRatio);

    if (dirLen > 0.0001f) {
      forwardRatio /= dirLen;
      upwardRatio /= dirLen;
    } else {
      forwardRatio = 0.707f;
      upwardRatio = 0.707f;
    }

    float pushX = pushMagnitude * forwardRatio;
    float pushY = pushMagnitude * upwardRatio;

    //==============================
    // 連続Perfect補正
    //==============================
    if (kickFeedbackType_ == KickFeedbackType::Perfect) {
      int streakLevel = std::min(perfectStreak_ - 1, 4);

      float streakForwardScale = 1.0f + streakLevel * 1.0f;
      float streakRiseScale = 1.0f - streakLevel * 1.0f;
      streakRiseScale = std::clamp(streakRiseScale, 0.80f, 1.0f);

      pushX *= streakForwardScale;
      pushY *= streakRiseScale;
    }

    velocityX_ += pushX;
    velocityY_ += pushY;
  }

  //==============================
  // 蹴りにならなかった側は減衰
  //==============================
  if (!(leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
        !requireReleaseAfterLandLeft_)) {
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_ - driveDecay_);
  }

  if (!(rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
        !requireReleaseAfterLandRight_)) {
    rightDriveAccum_ = (std::max)(0.0f, rightDriveAccum_ - driveDecay_);
  }

  //==============================
  // 姿勢更新
  //==============================
  float tiltDamping =
      std::clamp(0.82f + (torsoSizeScale_ - 1.0f) * 0.08f, 0.76f, 0.92f);
  bodyTiltVelocity_ *= tiltDamping;

  float headTiltRangeFactor =
      std::clamp(1.0f + (headSizeScale_ - 1.0f) * 1.60f, 1.0f, 3.20f);

  float torsoTiltRangeFactor =
      std::clamp(1.0f - (torsoSizeScale_ - 1.0f) * 0.45f, 0.65f, 1.10f);

  float dynamicForwardTilt =
      maxForwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor;
  float dynamicBackwardTilt =
      maxBackwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor;

  bodyTilt_ += bodyTiltVelocity_;
  bodyTilt_ = std::clamp(bodyTilt_, dynamicForwardTilt, dynamicBackwardTilt);

  if (debugForceTilt_) {
    bodyTilt_ = debugTiltValue_;
  }

  if (isRecoveringFromTilt_) {
    float recoverPower =
        0.010f * recoveryAssist_ * headRecoveryPenalty * torsoStabilityScale_;

    if (std::abs(bodyTilt_) > heavyFallTilt) {
      recoverPower *= 1.4f;
      velocityX_ *= 0.96f;
    }

    float tiltToTarget = recoverTargetTilt - bodyTilt_;
    bodyTiltVelocity_ += tiltToTarget * recoverPower;
  }

  //==============================
  // 接地中の drive 蓄積
  //==============================
  float leftExtendAmount = 0.0f;
  float rightExtendAmount = 0.0f;
  float leftRecoverAmount = 0.0f;
  float rightRecoverAmount = 0.0f;

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

  //==============================
  // 速度更新
  //==============================
  bool wasGrounded = isGrounded_;

  velocityX_ *= inertia_;

  float maxSpeed = useCustomizeMove_ ? tuning_.maxSpeed : 1.0f;
  velocityX_ = std::clamp(velocityX_, -1.2f * maxSpeed, 1.2f * maxSpeed);

  moveX_ += velocityX_;

  velocityY_ -= gravity_;
  moveY_ += velocityY_;

  // if (moveY_ <= groundY_) {
  //   isGrounded_ = true;
  //   moveY_ = groundY_;
  //   velocityY_ = 0.0f;
  // } else {
  //   isGrounded_ = false;
  // }

  if (cp != nullptr && customizeData_ != nullptr) {

    // const float lowestLegBottomLocalY =
    //     (std::min)(leftFootBottomLocalY, rightFootBottomLocalY);

    const float contactMoveY = groundY_ - lowestLegBottomLocalY;

    if (moveY_ <= contactMoveY + groundEpsilon) {
      isGrounded_ = true;
      moveY_ = contactMoveY;
      velocityY_ = 0.0f;
    } else {
      isGrounded_ = false;
    }

  } else {
    if (moveY_ <= groundY_) {
      isGrounded_ = true;
      moveY_ = groundY_;
      velocityY_ = 0.0f;
    } else {
      isGrounded_ = false;
    }
  }

  bool justLanded = (!wasGrounded && isGrounded_);

  if (justLanded) {
    if (leftNowInput) {
      aKeyFlashTimer_ = 0.15f;
    }
    if (rightNowInput) {
      dKeyFlashTimer_ = 0.15f;
    }
  }

  if (justLanded) {
    landTimer_ = 0.0f;
  } else if (isGrounded_) {
    landTimer_ += system_->GetDeltaTime();
  } else {
    landTimer_ = 999.0f;
  }

  if (justLanded) {
    leftDriveAccum_ = 0.0f;
    rightDriveAccum_ = 0.0f;

    leftHoldTime_ = 0.0f;
    rightHoldTime_ = 0.0f;

    requireReleaseAfterLandLeft_ = leftNowInput;
    requireReleaseAfterLandRight_ = rightNowInput;
  }

  bodyStretch_ *= 0.90f;

  leftPrevInput_ = leftNowInput;
  rightPrevInput_ = rightNowInput;
}

void TravelRunner::ApplyVisualState() {
  if (customizeData_ == nullptr) {
    return;
  }

  leftLegBendHistory_[animHistoryIndex_] = leftLegBend_;
  rightLegBendHistory_[animHistoryIndex_] = rightLegBend_;
  animHistoryIndex_ = (animHistoryIndex_ + 1) % kMaxAnimHistory;

  for (const auto &instance : customizeData_->partInstances) {
    auto it = allPartObjects_.find(instance.partId);
    if (it == allPartObjects_.end() || it->second == nullptr)
      continue;
    Object *obj = it->second;

    if (obj->objectParts_.empty())
      continue;

    // Base Translation from user customization (relative to parent, with
    // dynamic offsets applied)
    obj->mainPosition.transform.translate = instance.resolvedLocalTranslate;

    // Apply parent's scale to the local translation if parent is Torso
    // This fixes the issue where big-bodied NPCs have normal shoulder widths
    if (instance.parentId >= 0) {
      auto parentIt = allPartObjects_.find(instance.parentId);
      if (parentIt != allPartObjects_.end() && parentIt->second != nullptr) {
        if (!parentIt->second->objectParts_.empty()) {
          ModBodyPart parentType = ModBodyPart::Count;
          float parentParamScaleX = 1.0f;
          float parentParamScaleY = 1.0f;
          float parentParamScaleZ = 1.0f;
          float parentParamLength = 1.0f;

          for (const auto &pInst : customizeData_->partInstances) {
            if (pInst.partId == instance.parentId) {
              parentType = pInst.partType;
              parentParamScaleX = pInst.param.scale.x;
              parentParamScaleY = pInst.param.scale.y;
              parentParamScaleZ = pInst.param.scale.z;
              parentParamLength = pInst.param.length;
              break;
            }
          }
          if (parentType == ModBodyPart::ChestBody || parentType == ModBodyPart::StomachBody) {
            float parentScaleX = parentParamScaleX;
            float parentScaleY = parentParamScaleY * parentParamLength;
            float parentScaleZ = parentParamScaleZ;

            obj->mainPosition.transform.translate.x *= parentScaleX;

            // X scale moves the attach point outwards, but the torso's thickness also scales.
            // We use parentParamScaleX which correctly matches the user's manual width scaling.
            // Z scale works similarly.

            float parentSegLength = GetSnapshotSegmentLength(parentType, -1);
            if (parentSegLength <= 0.0001f) {
                parentSegLength = (parentType == ModBodyPart::ChestBody) ? 1.2796f : 1.6880f;
            }

            // The origin of the torso parts is at the TOP (bone root).
            // Scaling in Y only moves the BOTTOM edge (bone tip).
            // Therefore, parts attached to the top (Neck, Arms) DO NOT move in Y.
            // Parts attached to the bottom (Legs) move down by the full expansion amount.
            float shiftSign = 0.0f;
            if (instance.partType == ModBodyPart::LeftThigh || instance.partType == ModBodyPart::RightThigh) {
                shiftSign = -1.0f;
                
                // Because StomachBody's visual mesh is shifted down by ChestBody's expansion,
                // we must also shift the legs down by that same amount so they stay attached.
                float chestScaleY = 1.0f;
                for (const auto &pInst : customizeData_->partInstances) {
                    if (pInst.partType == ModBodyPart::ChestBody) {
                        chestScaleY = pInst.param.scale.y * pInst.param.length;
                        break;
                    }
                }
                float chestSegLength = GetSnapshotSegmentLength(ModBodyPart::ChestBody, -1);
                if (chestSegLength <= 0.0001f) chestSegLength = 1.2796f;
                obj->mainPosition.transform.translate.y -= (chestScaleY - 1.0f) * chestSegLength;
            }

            obj->mainPosition.transform.translate.y += shiftSign * (parentScaleY - 1.0f) * parentSegLength;
            obj->mainPosition.transform.translate.z *= parentScaleZ;
          }
        }
      }
    }

    // Root objects (like ChestBody or StomachBody with no parent) need world offsets added
    if (instance.parentId < 0) {
      obj->mainPosition.transform.translate.x += laneX_;
      obj->mainPosition.transform.translate.y += moveY_ + visualLiftY_;
      obj->mainPosition.transform.translate.z += moveX_;
    }

    ModBodyPart parentType = ModBodyPart::Count;
    if (instance.parentId >= 0) {
      for (const auto &pInst : customizeData_->partInstances) {
        if (pInst.partId == instance.parentId) {
          parentType = pInst.partType;
          break;
        }
      }

      bool isStandardJoint = false;
      if ((instance.partType == ModBodyPart::LeftForeArm &&
           parentType == ModBodyPart::LeftUpperArm) ||
          (instance.partType == ModBodyPart::RightForeArm &&
           parentType == ModBodyPart::RightUpperArm) ||
          (instance.partType == ModBodyPart::LeftShin &&
           parentType == ModBodyPart::LeftThigh) ||
          (instance.partType == ModBodyPart::RightShin &&
           parentType == ModBodyPart::RightThigh)) {
        isStandardJoint = true;
      }

      if (isStandardJoint) {
        auto parentIt = allPartObjects_.find(instance.parentId);
        if (parentIt != allPartObjects_.end() && parentIt->second != nullptr) {
          if (!parentIt->second->objectParts_.empty()) {
            float parentScaleY = 1.0f;
            for (const auto &pInst : customizeData_->partInstances) {
              if (pInst.partId == instance.parentId) {
                parentScaleY = pInst.param.scale.y * pInst.param.length;
                break;
              }
            }
            float parentDefaultLength = 1.0f;
            switch (parentType) {
            case ModBodyPart::LeftUpperArm:
            case ModBodyPart::RightUpperArm:
              parentDefaultLength = 1.08f;
              break;
            case ModBodyPart::LeftThigh:
            case ModBodyPart::RightThigh:
              parentDefaultLength = 1.57f;
              break;
            default:
              break;
            }
            obj->mainPosition.transform.translate = {
                0.0f, -(parentScaleY * parentDefaultLength), 0.0f};
          }
        }
      }
    }

    // Base Rotation
    float baseAngleX = instance.localTransform.rotate.x;
    float baseAngleY = instance.localTransform.rotate.y;
    float baseAngleZ = instance.localTransform.rotate.z;
    if (instance.partType != ModBodyPart::ChestBody &&
        instance.partType != ModBodyPart::StomachBody) {

      int snapshotOwnerId = GetExtraSnapshotOwnerId(
          instance.partType, instance.partId, instance.parentId);

      float absBaseX = 0.0f, absBaseZ = 0.0f;
      ComputeExtraBaseAngles(instance.partType, snapshotOwnerId, absBaseX,
                             absBaseZ);

      float parentAbsX = 0.0f, parentAbsZ = 0.0f;
      if (instance.parentId >= 0) {
        ModBodyPart parentType = ModBodyPart::Count;
        for (const auto &pInst : customizeData_->partInstances) {
          if (pInst.partId == instance.parentId) {
            parentType = pInst.partType;
            break;
          }
        }
        if (parentType != ModBodyPart::ChestBody &&
            parentType != ModBodyPart::StomachBody) {
          int parentOwnerId =
              GetExtraSnapshotOwnerId(parentType, instance.parentId, -1);
          ComputeExtraBaseAngles(parentType, parentOwnerId, parentAbsX,
                                 parentAbsZ);
        }
      }

      baseAngleX = absBaseX - parentAbsX;
      baseAngleZ = absBaseZ - parentAbsZ;
    }

    // Animation Rotation
    bool isGroupA = true;
    if (instance.partType == ModBodyPart::LeftThigh ||
        instance.partType == ModBodyPart::LeftUpperArm ||
        instance.partType == ModBodyPart::LeftShin ||
        instance.partType == ModBodyPart::LeftForeArm) {
      isGroupA = true;
    } else if (instance.partType == ModBodyPart::RightThigh ||
               instance.partType == ModBodyPart::RightUpperArm ||
               instance.partType == ModBodyPart::RightShin ||
               instance.partType == ModBodyPart::RightForeArm) {
      isGroupA = false;
    }

    float driveAngle = leftLegBend_;
    float oppositeAngle = rightLegBend_;

    int index = 0;
    if (customizeData_ != nullptr) {
      for (const auto &inst : customizeData_->partInstances) {
        if (inst.partType == instance.partType) {
          if (inst.partId == instance.partId)
            break;
          index++;
        }
      }
    }

    if (index > 0) {
      int delayFrames = index * 4; // ~4 frames delay per extra part
      if (delayFrames >= kMaxAnimHistory)
        delayFrames = kMaxAnimHistory - 1;

      int histIdx = (animHistoryIndex_ - 1 - delayFrames);
      if (histIdx < 0)
        histIdx += kMaxAnimHistory;

      driveAngle = leftLegBendHistory_[histIdx];
      oppositeAngle = rightLegBendHistory_[histIdx];
    }

    if (!isGroupA) {
      std::swap(driveAngle, oppositeAngle);
    }
    const float armSwingScale = 0.60f;
    const float thighSwingScale = 0.70f;
    float animAngleX = 0.0f;

    switch (instance.partType) {
    case ModBodyPart::ChestBody:
    case ModBodyPart::StomachBody: {
      bool parentIsTorso = false;
      if (instance.parentId >= 0) {
        for (const auto &pInst : customizeData_->partInstances) {
          if (pInst.partId == instance.parentId &&
              (pInst.partType == ModBodyPart::ChestBody ||
               pInst.partType == ModBodyPart::StomachBody)) {
            parentIsTorso = true;
            break;
          }
        }
      }
      if (!parentIsTorso) {
        animAngleX = -bodyTilt_;
      }
      break;
    }
    case ModBodyPart::LeftUpperArm:
    case ModBodyPart::RightUpperArm: {
      float swing = -oppositeAngle;
      if (swing > 0.0f)
        swing *= 1.6f;
      animAngleX = swing * armSwingScale;
      break;
    }
    case ModBodyPart::LeftForeArm:
    case ModBodyPart::RightForeArm: {
      float swing = -oppositeAngle;
      if (swing > 0.0f)
        swing *= 1.6f;
      float upperArmSwing = swing * armSwingScale;
      float elbowFold = std::clamp((oppositeAngle - legKickAngle_) /
                                       (legRecoverAngle_ - legKickAngle_),
                                   0.0f, 1.0f);
      animAngleX = -(upperArmSwing * 0.35f + elbowFold * 0.70f + 0.30f);
      break;
    }
    case ModBodyPart::LeftThigh:
    case ModBodyPart::RightThigh: {
      if (baseAngleX < 0.0f && driveAngle < 0.0f) {
        float kickRatio = driveAngle / legKickAngle_;
        baseAngleX = std::lerp(baseAngleX, 0.0f, kickRatio * 0.85f);
      }
      animAngleX = -driveAngle * thighSwingScale + bodyTilt_;
      break;
    }
    case ModBodyPart::LeftShin:
    case ModBodyPart::RightShin: {
      if (baseAngleX > 0.0f && driveAngle < 0.0f) {
        float kickRatio = driveAngle / legKickAngle_;
        baseAngleX = std::lerp(baseAngleX, 0.0f, kickRatio * 0.85f);
      }
      float thighSwing = -driveAngle * thighSwingScale;
      float kneeFold = std::clamp((driveAngle - legKickAngle_) /
                                      (legRecoverAngle_ - legKickAngle_),
                                  0.0f, 1.0f);
      animAngleX = thighSwing * 0.35f + kneeFold * 0.6f;
      break;
    }
    default:
      break;
    }

    obj->mainPosition.transform.rotate = {baseAngleX + animAngleX, baseAngleY,
                                          baseAngleZ};

    if (!obj->objectParts_.empty()) {
      int snapshotOwnerId = GetExtraSnapshotOwnerId(
          instance.partType, instance.partId, instance.parentId);
      float rootR = 0.1f, bendR = 0.1f, endR = 0.1f;
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId == snapshotOwnerId) {
          if (snap.role == ModControlPointRole::Root)
            rootR = snap.radius;
          else if (snap.role == ModControlPointRole::Bend)
            bendR = snap.radius;
          else if (snap.role == ModControlPointRole::End)
            endR = snap.radius;
        }
      }
      if (instance.partType == ModBodyPart::ChestBody) {
        rootR = GetControlPointRadius(ModControlPointRole::Chest);
        bendR = GetControlPointRadius(ModControlPointRole::Belly);
      } else if (instance.partType == ModBodyPart::StomachBody) {
        rootR = GetControlPointRadius(ModControlPointRole::Waist);
        bendR = GetControlPointRadius(ModControlPointRole::Belly);
      } else if (instance.partType == ModBodyPart::Head) {
        rootR = GetControlPointRadius(ModControlPointRole::HeadCenter);
        bendR = GetControlPointRadius(ModControlPointRole::UpperNeck);
        endR = bendR;
      }

      float thicknessScale = 1.0f;
      if (instance.partType == ModBodyPart::LeftForeArm ||
          instance.partType == ModBodyPart::RightForeArm ||
          instance.partType == ModBodyPart::LeftShin ||
          instance.partType == ModBodyPart::RightShin ||
          instance.partType == ModBodyPart::Head) {
        thicknessScale = (std::max)(bendR, endR) / 0.1f;
      } else {
        thicknessScale = (std::max)(rootR, bendR) / 0.1f;
      }

      float originalSegLength =
          GetSnapshotSegmentLength(instance.partType, snapshotOwnerId);
      bool isIKDriven = (originalSegLength > 0.0001f);

      float segLength = originalSegLength;
      if (!isIKDriven)
        segLength = 1.0f;

      float defaultSegLength = 1.0f;
      switch (instance.partType) {
      case ModBodyPart::ChestBody:
        defaultSegLength = 1.2796f;
        break;
      case ModBodyPart::StomachBody:
        defaultSegLength = 1.6880f;
        break;
      case ModBodyPart::Neck:
        defaultSegLength = 0.3462f;
        break;
      case ModBodyPart::Head:
        defaultSegLength = 1.6790f;
        break;
      case ModBodyPart::LeftUpperArm:
      case ModBodyPart::RightUpperArm:
        defaultSegLength = 1.08f;
        break;
      case ModBodyPart::LeftForeArm:
      case ModBodyPart::RightForeArm:
        defaultSegLength = 2.14f;
        break;
      case ModBodyPart::LeftThigh:
      case ModBodyPart::RightThigh:
        defaultSegLength = 1.57f;
        break;
      case ModBodyPart::LeftShin:
      case ModBodyPart::RightShin:
        defaultSegLength = 2.62f;
        break;
      default:
        break;
      }

      float finalScaleY = instance.param.scale.y * instance.param.length * 1.0f;
      if (isIKDriven) {
        if (defaultSegLength > 0.0001f) {
          finalScaleY *= (segLength / defaultSegLength);
        }
      }

      obj->objectParts_[0].transform.scale.x =
          thicknessScale * instance.param.scale.x;
      obj->objectParts_[0].transform.scale.y = finalScaleY;
      obj->objectParts_[0].transform.scale.z =
          thicknessScale * instance.param.scale.z;

      if (!isIKDriven) {
        if (instance.partType == ModBodyPart::LeftUpperArm ||
            instance.partType == ModBodyPart::LeftForeArm ||
            instance.partType == ModBodyPart::RightUpperArm ||
            instance.partType == ModBodyPart::RightForeArm ||
            instance.partType == ModBodyPart::LeftThigh ||
            instance.partType == ModBodyPart::LeftShin ||
            instance.partType == ModBodyPart::RightThigh ||
            instance.partType == ModBodyPart::RightShin) {
          obj->objectParts_[0].transform.translate = {0.0f, -finalScaleY * 0.5f,
                                                      0.0f};
        } else if (instance.partType == ModBodyPart::Head ||
                   instance.partType == ModBodyPart::Neck) {
          obj->objectParts_[0].transform.translate = {0.0f, finalScaleY * 0.5f,
                                                      0.0f};
        } else {
          obj->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
        }
      } else {
        Vector3 startPos = {0.0f, 0.0f, 0.0f};
        if (instance.partType == ModBodyPart::ChestBody) {
          for (const auto &snap : customizeData_->controlPointSnapshots) {
            if (snap.role == ModControlPointRole::Chest) {
              startPos = snap.localPosition;
              break;
            }
          }
        } else if (instance.partType == ModBodyPart::StomachBody) {
          for (const auto &snap : customizeData_->controlPointSnapshots) {
            if (snap.role == ModControlPointRole::Belly) {
              startPos = snap.localPosition;
              break;
            }
          }
          // Shift StomachBody's local mesh down so it rotates around the same pivot as ChestBody!
          float chestScaleY = 1.0f;
          for (const auto &pInst : customizeData_->partInstances) {
              if (pInst.partType == ModBodyPart::ChestBody) {
                  chestScaleY = pInst.param.scale.y * pInst.param.length;
                  break;
              }
          }
          float chestSegLength = GetSnapshotSegmentLength(ModBodyPart::ChestBody, -1);
          if (chestSegLength <= 0.0001f) chestSegLength = 1.2796f;
          
          startPos.y -= (chestScaleY - 1.0f) * chestSegLength;
        }
        obj->objectParts_[0].transform.translate = startPos;
      }
    }
  }

  UpdateModObjects();
  ResolveVisualGroundPenetration();
}
void TravelRunner::ResolveVisualGroundPenetration() {

  LowestBodyPart lowestPart = LowestBodyPart::None;
  float lowestBodyLocalY = GetLowestVisualBodyY(&lowestPart);
  float lowestBodyWorldY = moveY_ + visualLiftY_ + lowestBodyLocalY;
  float penetration = groundY_ - lowestBodyWorldY;

  float requiredLift = visualLiftY_ + penetration;
  const float maxVisualLift = 6.0f;
  float targetLift = std::clamp(requiredLift, 0.0f, maxVisualLift);

  float follow = 0.40f;
  if (penetration > 0.80f) {
    follow = 0.75f;
  } else if (penetration > 0.30f) {
    follow = 0.55f;
  } else if (penetration <= 0.0f) {
    follow = 0.15f; // Slower decay when airborne to prevent sudden drops
  }

  visualLiftY_ += (targetLift - visualLiftY_) * follow;

  if (std::abs(visualLiftY_) < 0.001f) {
    visualLiftY_ = 0.0f;
  }
}

void TravelRunner::BuildFeaturesFromCustomizeData() {
  features_ = {};

  if (customizeData_ == nullptr) {
    return;
  }

  for (const auto &instance : customizeData_->partInstances) {

    if (instance.partType == ModBodyPart::Head) {
      features_.headCount++;
    }

    if (instance.partType == ModBodyPart::LeftUpperArm ||
        instance.partType == ModBodyPart::RightUpperArm) {
      features_.armCount++;
    }

    if (instance.partType == ModBodyPart::LeftThigh ||
        instance.partType == ModBodyPart::RightThigh) {
      features_.legCount++;
    }

    features_.centerOfMassY += instance.localTransform.translate.y;
    features_.asymmetry += instance.localTransform.translate.x;
    features_.lowestPoint =
        std::min(features_.lowestPoint, instance.localTransform.translate.y);
  }
  features_.asymmetry = std::abs(features_.asymmetry);
}

void TravelRunner::CollectSnapshotsByOwnerId(
    int ownerPartId,
    std::vector<const ModControlPointSnapshot *> &outSnapshots) const {

  outSnapshots.clear();

  if (customizeData_ == nullptr) {
    return;
  }

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartId == ownerPartId) {
      outSnapshots.push_back(&snap);
    }
  }
}

float TravelRunner::GetLowestVisualBodyY(LowestBodyPart *outPart) const {
  auto UpdateLowest = [&](float candidateY, LowestBodyPart part, float &lowestY,
                          LowestBodyPart &lowestPart) {
    if (candidateY < lowestY) {
      lowestY = candidateY;
      lowestPart = part;
    }
  };

  float lowestY = 999999.0f;
  LowestBodyPart lowestPart = LowestBodyPart::None;

  auto GetTipWorldY = [&](Object *obj, float defaultMeshLength,
                          float radius) -> float {
    if (obj == nullptr || obj->objectParts_.empty()) {
      return 999999.0f;
    }
    Matrix4x4 localMat =
        MakeAffineMatrix(obj->objectParts_[0].transform.scale,
                         obj->objectParts_[0].transform.rotate,
                         obj->objectParts_[0].transform.translate);

    Vector3 tipPoint = {0.0f, -defaultMeshLength, 0.0f};
    Vector3 tipInMainPos = ModObjectUtil::TransformPoint(localMat, tipPoint);
    Matrix4x4 mainPosWorld = ModObjectUtil::ComputeMainPositionWorldMatrix(obj);
    Vector3 tipWorld =
        ModObjectUtil::TransformPoint(mainPosWorld, tipInMainPos);

    return tipWorld.y - radius;
  };

  auto GetPartBottomWorldY = [&](Object *obj, float radius) -> float {
    if (obj == nullptr || obj->objectParts_.empty()) {
      return 999999.0f;
    }
    Matrix4x4 mainPosWorld = ModObjectUtil::ComputeMainPositionWorldMatrix(obj);
    Vector3 centerWorld = {mainPosWorld.m[3][0], mainPosWorld.m[3][1],
                           mainPosWorld.m[3][2]};
    return centerWorld.y - radius;
  };

  if (customizeData_ != nullptr) {
    const float leftRadius = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);
    const float rightRadius = GetSnapshotRadius(ModBodyPart::RightThigh, 3);

    for (const auto &inst : customizeData_->partInstances) {
      if (inst.partType == ModBodyPart::LeftShin) {
        auto it = allPartObjects_.find(inst.partId);
        if (it != allPartObjects_.end() && it->second != nullptr) {
          UpdateLowest(GetTipWorldY(it->second, 2.6181f, leftRadius),
                       LowestBodyPart::LeftShin, lowestY, lowestPart);
        }
      } else if (inst.partType == ModBodyPart::RightShin) {
        auto it = allPartObjects_.find(inst.partId);
        if (it != allPartObjects_.end() && it->second != nullptr) {
          UpdateLowest(GetTipWorldY(it->second, 2.6181f, rightRadius),
                       LowestBodyPart::RightShin, lowestY, lowestPart);
        }
      }
    }
  } else {
    if (Object *leftShin = GetStandardPart(ModBodyPart::LeftShin)) {
      const float r = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);
      UpdateLowest(GetTipWorldY(leftShin, 2.6181f, r), LowestBodyPart::LeftShin,
                   lowestY, lowestPart);
    }
    if (Object *rightShin = GetStandardPart(ModBodyPart::RightShin)) {
      const float r = GetSnapshotRadius(ModBodyPart::RightThigh, 3);
      UpdateLowest(GetTipWorldY(rightShin, 2.6181f, r),
                   LowestBodyPart::RightShin, lowestY, lowestPart);
    }
  }

  if (outPart != nullptr) {
    *outPart = lowestPart;
  }

  // Convert the calculated absolute world Y back to a relative local Y offset
  // from the player's root coordinate
  return lowestY - (moveY_ + visualLiftY_);
}
const char *TravelRunner::GetLowestBodyPartName(LowestBodyPart part) const {
  switch (part) {
  case LowestBodyPart::LeftForeArm:
    return "LeftForeArm";
  case LowestBodyPart::RightForeArm:
    return "RightForeArm";
  case LowestBodyPart::LeftShin:
    return "LeftShin";
  case LowestBodyPart::RightShin:
    return "RightShin";
  case LowestBodyPart::Head:
    return "Head";
  case LowestBodyPart::Chest:
    return "Chest";
  case LowestBodyPart::Stomach:
    return "Stomach";
  default:
    return "None";
  }
}

bool TravelRunner::HasRequiredParts() const {
  Object *chestBody = GetStandardPart(ModBodyPart::ChestBody);
  Object *stomachBody = GetStandardPart(ModBodyPart::StomachBody);
  Object *neck = GetStandardPart(ModBodyPart::Neck);
  Object *head = GetStandardPart(ModBodyPart::Head);

  Object *leftUpperArm = GetStandardPart(ModBodyPart::LeftUpperArm);
  Object *leftForeArm = GetStandardPart(ModBodyPart::LeftForeArm);
  Object *rightUpperArm = GetStandardPart(ModBodyPart::RightUpperArm);
  Object *rightForeArm = GetStandardPart(ModBodyPart::RightForeArm);

  Object *leftThigh = GetStandardPart(ModBodyPart::LeftThigh);
  Object *leftShin = GetStandardPart(ModBodyPart::LeftShin);
  Object *rightThigh = GetStandardPart(ModBodyPart::RightThigh);
  Object *rightShin = GetStandardPart(ModBodyPart::RightShin);

  // nullチェック
  if (chestBody == nullptr || neck == nullptr || head == nullptr ||
      leftUpperArm == nullptr || leftForeArm == nullptr ||
      rightUpperArm == nullptr || rightForeArm == nullptr ||
      leftThigh == nullptr || leftShin == nullptr || rightThigh == nullptr ||
      rightShin == nullptr) {
    return false;
  }

  return true;
}

float TravelRunner::GetSnapshotRadius(ModBodyPart ownerPart,
                                      int localRole) const {
  if (customizeData_ == nullptr) {
    return 0.1f;
  }

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartType == ownerPart &&
        static_cast<int>(snap.role) == localRole) {
      return snap.radius;
    }
  }

  return 0.1f;
}

float TravelRunner::GetControlPointRadius(ModControlPointRole role) const {
  if (customizeData_ == nullptr) {
    return 0.1f;
  }

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.role == role) {
      return snap.radius;
    }
  }

  return 0.1f;
}

const ModControlPointData *TravelRunner::GetControlPoints() const {
  if (customizeData_ == nullptr) {
    return nullptr;
  }
  return &customizeData_->controlPoints;
}

bool TravelRunner::BuildSegmentFromSnapshot(ModBodyPart partType, int partId,
                                            SegmentVisual &out) {
  if (!customizeData_)
    return false;

  const auto &snaps = customizeData_->controlPointSnapshots;

  Vector3 rootPos{};
  Vector3 midPos{};
  Vector3 endPos{};

  bool hasRoot = false;
  bool hasBend = false;
  bool hasEnd = false;

  for (const auto &s : snaps) {
    if (s.ownerPartId != partId)
      continue;

    switch (s.role) {
    case ModControlPointRole::Root:
      rootPos = s.localPosition;
      hasRoot = true;
      break;
    case ModControlPointRole::Bend:
      midPos = s.localPosition;
      hasBend = true;
      break;
    case ModControlPointRole::End:
      endPos = s.localPosition;
      hasEnd = true;
      break;
    default:
      break;
    }
  }

  Vector3 start{};
  Vector3 end{};

  if (partType == ModBodyPart::LeftUpperArm ||
      partType == ModBodyPart::RightUpperArm ||
      partType == ModBodyPart::LeftThigh ||
      partType == ModBodyPart::RightThigh || partType == ModBodyPart::Neck) {
    if (!hasRoot || !hasBend)
      return false;
    start = rootPos;
    end = midPos;
  } else if (partType == ModBodyPart::LeftForeArm ||
             partType == ModBodyPart::RightForeArm ||
             partType == ModBodyPart::LeftShin ||
             partType == ModBodyPart::RightShin ||
             partType == ModBodyPart::Head) {
    if (!hasBend || !hasEnd)
      return false;
    start = midPos;
    end = endPos;
  } else {
    return false;
  }

  Vector3 diff = {end.x - start.x, end.y - start.y, end.z - start.z};

  float length = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
  if (length < 0.0001f) {
    return false;
  }

  Vector3 dir = {diff.x / length, diff.y / length, diff.z / length};

  float angleZ = atan2f(dir.x, -dir.y);
  float angleX = -asinf(std::clamp(dir.z, -1.0f, 1.0f));

  out.root = start;
  out.length = length;
  out.angleX = angleX;
  out.angleZ = angleZ;
  out.thickness = 1.0f;

  return true;
}
Vector3 TravelRunner::BuildAnimatedChildRootFromParent(const Vector3 &root,
                                                       float angleZ,
                                                       float angleX,
                                                       float length) const {
  Vector3 result{};
  result.x = root.x + std::sin(angleZ) * std::cos(angleX) * length;
  result.y = root.y - std::cos(angleZ) * std::cos(angleX) * length;
  result.z = root.z - std::sin(angleX) * length;
  return result;
}

bool TravelRunner::GetExtraPartSnapshotPositions(int partId, Vector3 &outRoot,
                                                 Vector3 &outBend,
                                                 Vector3 &outEnd) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  bool foundAny = false;

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartId != partId) {
      continue;
    }

    if (snap.role == ModControlPointRole::Root) {
      outRoot = snap.localPosition;
      foundAny = true;
    } else if (snap.role == ModControlPointRole::Bend) {
      outBend = snap.localPosition;
      foundAny = true;
    } else if (snap.role == ModControlPointRole::End) {
      outEnd = snap.localPosition;
      foundAny = true;
    }
  }

  return foundAny;
}

bool TravelRunner::GetExtraInstanceLocalTranslate(int partId,
                                                  Vector3 &outLocal) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId == partId) {
      outLocal = instance.localTransform.translate;
      return true;
    }
  }

  return false;
}

bool TravelRunner::GetFirstPartTypePartId(ModBodyPart partType,
                                          int &outPartId) const {
  outPartId = -1;

  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partType == partType) {
      outPartId = instance.partId;
      return true;
    }
  }

  return false;
}

float TravelRunner::GetSnapshotSegmentLength(ModBodyPart partType,
                                             int ownerPartId) const {
  auto GetTorsoSnap = [&](ModControlPointRole role, Vector3 &outPos) -> bool {
    if (customizeData_ == nullptr)
      return false;
    for (const auto &snap : customizeData_->controlPointSnapshots) {
      if (snap.role == role) {
        outPos = snap.localPosition;
        return true;
      }
    }
    return false;
  };

  if (partType == ModBodyPart::ChestBody) {
    Vector3 chest = {0, 0, 0}, belly = {0, 0, 0};
    GetTorsoSnap(ModControlPointRole::Chest, chest);
    GetTorsoSnap(ModControlPointRole::Belly, belly);
    return Length(Subtract(belly, chest));
  } else if (partType == ModBodyPart::StomachBody) {
    Vector3 belly = {0, 0, 0}, waist = {0, 0, 0};
    GetTorsoSnap(ModControlPointRole::Belly, belly);
    GetTorsoSnap(ModControlPointRole::Waist, waist);
    return Length(Subtract(waist, belly));
  }

  Vector3 snapRoot = {0.0f, 0.0f, 0.0f};
  Vector3 snapBend = {0.0f, 0.0f, 0.0f};
  Vector3 snapEnd = {0.0f, 0.0f, 0.0f};

  if (!GetExtraPartSnapshotPositions(ownerPartId, snapRoot, snapBend,
                                     snapEnd)) {
    return 0.0f;
  }

  Vector3 from = snapRoot;
  Vector3 to = snapBend;

  switch (partType) {
  case ModBodyPart::Head:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    from = snapBend;
    to = snapEnd;
    break;

  default:
    from = snapRoot;
    to = snapBend;
    break;
  }

  return Length({to.x - from.x, to.y - from.y, to.z - from.z});
}

bool TravelRunner::GetPartInstanceParentId(int partId, int &outParentId) const {
  outParentId = -1;

  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId == partId) {
      outParentId = instance.parentId;
      return true;
    }
  }

  return false;
}

bool TravelRunner::GetExtraPartParentObject(
    ModBodyPart partType, int parentId,
    const std::unordered_map<int, Object *> &extraPartObjectMap,
    Object *&outParent) const {
  outParent = nullptr;

  auto it = extraPartObjectMap.find(parentId);
  if (it != extraPartObjectMap.end()) {
    outParent = it->second;
    return true;
  }

  switch (partType) {
  case ModBodyPart::LeftForeArm:
    outParent = GetStandardPart(ModBodyPart::LeftUpperArm);
    return outParent != nullptr;

  case ModBodyPart::RightForeArm:
    outParent = GetStandardPart(ModBodyPart::RightUpperArm);
    return outParent != nullptr;

  case ModBodyPart::LeftShin:
    outParent = GetStandardPart(ModBodyPart::LeftThigh);
    return outParent != nullptr;

  case ModBodyPart::RightShin:
    outParent = GetStandardPart(ModBodyPart::RightThigh);
    return outParent != nullptr;

  default:
    return false;
  }
}

bool TravelRunner::GetPartInstanceLocalTranslate(int partId,
                                                 Vector3 &outLocal) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId == partId) {
      outLocal = instance.localTransform.translate;
      return true;
    }
  }

  return false;
}

bool TravelRunner::GetPartInstanceLocalRotate(int partId,
                                              Vector3 &outRotate) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId != partId) {
      continue;
    }

    outRotate = instance.localTransform.rotate;
    return true;
  }

  return false;
}

bool TravelRunner::GetFirstPartTypeLocalTranslate(ModBodyPart partType,
                                                  Vector3 &outLocal) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partType == partType) {
      outLocal = instance.localTransform.translate;
      return true;
    }
  }

  return false;
}

int TravelRunner::GetExtraSnapshotOwnerId(ModBodyPart partType, int partId,
                                          int parentId) const {
  ModBodyPart targetOwnerPart = ModBodyPart::Count;
  switch (partType) {
  case ModBodyPart::Head:
    targetOwnerPart = ModBodyPart::Neck;
    break;
  case ModBodyPart::LeftForeArm:
    targetOwnerPart = ModBodyPart::LeftUpperArm;
    break;
  case ModBodyPart::RightForeArm:
    targetOwnerPart = ModBodyPart::RightUpperArm;
    break;
  case ModBodyPart::LeftShin:
    targetOwnerPart = ModBodyPart::LeftThigh;
    break;
  case ModBodyPart::RightShin:
    targetOwnerPart = ModBodyPart::RightThigh;
    break;
  default:
    return partId;
  }

  if (customizeData_ != nullptr) {
    int currentParentId = parentId;
    while (currentParentId >= 0) {
      bool found = false;
      for (const auto &instance : customizeData_->partInstances) {
        if (instance.partId == currentParentId) {
          if (instance.partType == targetOwnerPart) {
            return currentParentId;
          }
          currentParentId = instance.parentId;
          found = true;
          break;
        }
      }
      if (!found)
        break;
    }
  }

  return partId;
}

bool TravelRunner::ComputeExtraBaseAngles(ModBodyPart partType,
                                          int snapshotOwnerId,
                                          float &outBaseAngleX,
                                          float &outBaseAngleZ) const {
  Vector3 snapRoot = {0.0f, 0.0f, 0.0f};
  Vector3 snapBend = {0.0f, -0.5f, 0.0f};
  Vector3 snapEnd = {0.0f, -1.0f, 0.0f};

  if (!GetExtraPartSnapshotPositions(snapshotOwnerId, snapRoot, snapBend,
                                     snapEnd)) {
    return false;
  }

  Vector3 from = snapRoot;
  Vector3 to = snapBend;

  switch (partType) {
  case ModBodyPart::Head:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    from = snapBend;
    to = snapEnd;
    break;

  default:
    from = snapRoot;
    to = snapBend;
    break;
  }

  Vector3 vec = {to.x - from.x, to.y - from.y, to.z - from.z};
  if (partType == ModBodyPart::Head || partType == ModBodyPart::Neck) {
    vec.x = -vec.x;
    vec.y = -vec.y;
    vec.z = -vec.z;
  }
  float length = Length(vec);

  if (length < 0.0001f) {
    vec = {0.0f, -1.0f, 0.0f};
  } else {
    vec = Normalize(vec);
  }

  outBaseAngleZ = atan2(vec.x, -vec.y);
  outBaseAngleX = -asinf(std::clamp(vec.z, -1.0f, 1.0f));
  return true;
}

float TravelRunner::ComputeExtraAnimAngleX(ModBodyPart partType) const {
  const float armSwingScale = 0.60f;
  const float thighSwingScale = 0.70f;

  switch (partType) {
  case ModBodyPart::LeftUpperArm: {
    float swing = -rightLegBend_;
    if (swing > 0.0f)
      swing *= 1.6f;
    return swing * armSwingScale;
  }

  case ModBodyPart::RightUpperArm: {
    float swing = -leftLegBend_;
    if (swing > 0.0f)
      swing *= 1.6f;
    return swing * armSwingScale;
  }

  case ModBodyPart::LeftForeArm: {
    float swing = -rightLegBend_;
    if (swing > 0.0f)
      swing *= 1.6f;
    float upperArmSwing = swing * armSwingScale;
    float elbowFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                     (legRecoverAngle_ - legKickAngle_),
                                 0.0f, 1.0f);
    return -(upperArmSwing * 0.35f + elbowFold * 0.70f + 0.30f);
  }

  case ModBodyPart::RightForeArm: {
    float swing = -leftLegBend_;
    if (swing > 0.0f)
      swing *= 1.6f;
    float upperArmSwing = swing * armSwingScale;
    float elbowFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                     (legRecoverAngle_ - legKickAngle_),
                                 0.0f, 1.0f);
    return -(upperArmSwing * 0.35f + elbowFold * 0.70f + 0.30f);
  }

  case ModBodyPart::LeftThigh:
    return -leftLegBend_ * thighSwingScale;

  case ModBodyPart::RightThigh:
    return -rightLegBend_ * thighSwingScale;

  case ModBodyPart::LeftShin: {
    float thighSwing = -leftLegBend_ * thighSwingScale;
    float kneeFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                    (legRecoverAngle_ - legKickAngle_),
                                0.0f, 1.0f);
    return thighSwing * 0.35f + kneeFold * 0.6f;
  }

  case ModBodyPart::RightShin: {
    float thighSwing = -rightLegBend_ * thighSwingScale;
    float kneeFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                    (legRecoverAngle_ - legKickAngle_),
                                0.0f, 1.0f);
    return thighSwing * 0.35f + kneeFold * 0.6f;
  }

  default:
    return 0.0f;
  }
}

void TravelRunner::BuildAllVisualParts() {
  for (auto &pair : allPartObjects_) {
    if (pair.second)
      delete pair.second;
  }
  allPartObjects_.clear();
  if (customizeData_ == nullptr)
    return;
  auto GetModelPath = [](ModBodyPart partType) -> const char * {
    switch (partType) {
    case ModBodyPart::ChestBody:
      return "GAME/resources/modBody/chest/chest.obj";
    case ModBodyPart::StomachBody:
      return "GAME/resources/modBody/stomach/stomach.obj";
    case ModBodyPart::Head:
      return "GAME/resources/modBody/head/head.obj";
    case ModBodyPart::Neck:
      return "GAME/resources/modBody/neck/neck.obj";
    case ModBodyPart::RightUpperArm:
      return "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj";
    case ModBodyPart::RightForeArm:
      return "GAME/resources/modBody/rightForeArm/rightForeArm.obj";
    case ModBodyPart::LeftUpperArm:
      return "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj";
    case ModBodyPart::LeftForeArm:
      return "GAME/resources/modBody/leftForeArm/leftForeArm.obj";
    case ModBodyPart::LeftThigh:
      return "GAME/resources/modBody/leftThighs/leftThighs.obj";
    case ModBodyPart::LeftShin:
      return "GAME/resources/modBody/leftShin/leftShin.obj";
    case ModBodyPart::RightThigh:
      return "GAME/resources/modBody/rightThighs/rightThighs.obj";
    case ModBodyPart::RightShin:
      return "GAME/resources/modBody/rightShin/rightShin.obj";
    default:
      return nullptr;
    }
  };
  for (const auto &instance : customizeData_->partInstances) {
    const char *modelPath = GetModelPath(instance.partType);
    if (modelPath == nullptr)
      continue;
    const int modelHandle = system_->SetModelObj(modelPath);
    Object *obj = new Object;
    obj->IntObject(system_);
    obj->CreateModelData(modelHandle);
    obj->mainPosition.transform = CreateDefaultTransform();
    if (!obj->objectParts_.empty()) {
      obj->objectParts_[0].transform = CreateDefaultTransform();
    }
    allPartObjects_[instance.partId] = obj;
  }
  for (const auto &instance : customizeData_->partInstances) {
    auto it = allPartObjects_.find(instance.partId);
    if (it == allPartObjects_.end())
      continue;
    Object *obj = it->second;
    auto parentIt = allPartObjects_.find(instance.parentId);
    if (parentIt != allPartObjects_.end() && parentIt->second != nullptr) {
      obj->followObject_ = &parentIt->second->mainPosition;
      obj->mainPosition.parentPart = &parentIt->second->mainPosition;
    } else {
      obj->followObject_ = nullptr;
      obj->mainPosition.parentPart = nullptr;
    }
  }
}

/*

    if (source == nullptr) {
      obj->Update(camera);
      continue;
    }

    if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
      obj->objectParts_[0].transform.rotate =
          source->objectParts_[0].transform.rotate;
    }

    //==============================
    // 首は ChestBody 基準のワールド位置で置く
    // 頭は首の先端から生やす
    //==============================
    if (partType == ModBodyPart::Neck) {
      Object *chestBody = GetStandardPart(ModBodyPart::ChestBody);

      int extraOwnerId = GetExtraSnapshotOwnerId(partType, partId, parentId);

      Vector3 extraSnapRoot = {0.0f, 0.0f, 0.0f};
      Vector3 extraSnapBend = {0.0f, 0.0f, 0.0f};
      Vector3 extraSnapEnd = {0.0f, 0.0f, 0.0f};

      bool hasExtraSnap = GetExtraPartSnapshotPositions(
          extraOwnerId, extraSnapRoot, extraSnapBend, extraSnapEnd);

      Vector3 baseSnapRoot = {0.0f, 0.0f, 0.0f};
      Vector3 baseSnapBend = {0.0f, 0.0f, 0.0f};
      Vector3 baseSnapEnd = {0.0f, 0.0f, 0.0f};

      int basePartId = -1;
      bool hasBasePartId = GetFirstPartTypePartId(partType, basePartId);

      bool hasBaseSnap = false;
      if (hasBasePartId) {
        int baseParentId = -1;
        GetPartInstanceParentId(basePartId, baseParentId);

        int baseOwnerId =
            GetExtraSnapshotOwnerId(partType, basePartId, baseParentId);

        hasBaseSnap = GetExtraPartSnapshotPositions(baseOwnerId, baseSnapRoot,
                                                    baseSnapBend, baseSnapEnd);
      }

      if (chestBody != nullptr) {
        const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
        const Vector3 &baseNeckLocal = source->mainPosition.transform.translate;

        Vector3 snapDelta = {0.0f, 0.0f, 0.0f};
        if (hasExtraSnap && hasBaseSnap) {
          snapDelta = {extraSnapBend.x - baseSnapBend.x,
                       extraSnapBend.y - baseSnapBend.y,
                       extraSnapBend.z - baseSnapBend.z};
        }

        obj->mainPosition.transform.translate = {
            chestPos.x + baseNeckLocal.x + snapDelta.x,
            chestPos.y + baseNeckLocal.y + snapDelta.y,
            chestPos.z + baseNeckLocal.z + snapDelta.z};

      } else {
        obj->mainPosition.transform.translate =
            source->mainPosition.transform.translate;
      }

      Vector3 extraNeckRotate = {0.0f, 0.0f, 0.0f};
      if (GetPartInstanceLocalRotate(partId, extraNeckRotate)) {
        obj->mainPosition.transform.rotate = extraNeckRotate;
      } else {
        obj->mainPosition.transform.rotate =
            source->mainPosition.transform.rotate;
      }

      if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
        obj->objectParts_[0].transform.scale =
            source->objectParts_[0].transform.scale;
        obj->objectParts_[0].transform.translate =
            source->objectParts_[0].transform.translate;
        obj->objectParts_[0].transform.rotate =
            source->objectParts_[0].transform.rotate;

        if (hasExtraSnap && hasBaseSnap) {
          float extraSegmentLength =
              Length({extraSnapBend.x - extraSnapRoot.x,
                      extraSnapBend.y - extraSnapRoot.y,
                      extraSnapBend.z - extraSnapRoot.z});

          float baseSegmentLength = Length({baseSnapBend.x - baseSnapRoot.x,
                                            baseSnapBend.y - baseSnapRoot.y,
                                            baseSnapBend.z - baseSnapRoot.z});

          if (baseSegmentLength > 0.0001f) {
            float lengthRatio = extraSegmentLength / baseSegmentLength;
            obj->objectParts_[0].transform.scale.y *= lengthRatio;
            obj->objectParts_[0].transform.translate.y *= lengthRatio;
          }
        }
      }

      obj->Update(camera);
      continue;
    }

    if (partType == ModBodyPart::Head) {
      Object *parentObj = nullptr;

      auto it = extraPartObjectMap.find(parentId);
      if (it != extraPartObjectMap.end()) {
        parentObj = it->second;
      }

      if (parentObj != nullptr) {
        const Vector3 parentRoot = parentObj->mainPosition.transform.translate;

        obj->mainPosition.transform.translate = {
            parentRoot.x + baseHeadOffset.x, parentRoot.y + baseHeadOffset.y,
            parentRoot.z + baseHeadOffset.z};

      } else {
        Object *chestBody = GetStandardPart(ModBodyPart::ChestBody);

        if (chestBody != nullptr && fixedHead != nullptr) {
          const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
          const Vector3 &baseHeadLocal =
              fixedHead->mainPosition.transform.translate;

          obj->mainPosition.transform.translate = {
              chestPos.x + baseHeadLocal.x, chestPos.y + baseHeadLocal.y,
              chestPos.z + baseHeadLocal.z};
        } else {
          obj->mainPosition.transform.translate =
              source->mainPosition.transform.translate;
        }
      }

      Vector3 extraHeadRotate = {0.0f, 0.0f, 0.0f};
      if (GetPartInstanceLocalRotate(partId, extraHeadRotate)) {
        obj->mainPosition.transform.rotate = extraHeadRotate;
      } else {
        obj->mainPosition.transform.rotate =
            source->mainPosition.transform.rotate;
      }

      if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
        obj->objectParts_[0].transform.scale =
            source->objectParts_[0].transform.scale;
        obj->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
        obj->objectParts_[0].transform.rotate =
            source->objectParts_[0].transform.rotate;
      }

      obj->Update(camera);
      continue;
    }

    int snapshotOwnerId = GetExtraSnapshotOwnerId(partType, partId, parentId);

    float baseAngleX = 0.0f;
    float baseAngleZ = 0.0f;
    bool hasBaseAngles = ComputeExtraBaseAngles(partType, snapshotOwnerId,
                                                baseAngleX, baseAngleZ);

    if (!hasBaseAngles) {
      obj->Update(camera);
      continue;
    }

    Vector3 snapRoot = {0.0f, 0.0f, 0.0f};
    Vector3 snapBend = {0.0f, 0.0f, 0.0f};
    Vector3 snapEnd = {0.0f, 0.0f, 0.0f};

    bool hasSnapPositions = GetExtraPartSnapshotPositions(
        snapshotOwnerId, snapRoot, snapBend, snapEnd);

    if (!hasSnapPositions) {
      obj->Update(camera);
      continue;
    }

    Vector3 extraLocal = {0.0f, 0.0f, 0.0f};
    Vector3 baseLocal = {0.0f, 0.0f, 0.0f};

    bool hasExtraLocal = GetPartInstanceLocalTranslate(partId, extraLocal);
    bool hasBaseLocal = GetFirstPartTypeLocalTranslate(partType, baseLocal);

    Vector3 localDelta = {0.0f, 0.0f, 0.0f};
    if (hasExtraLocal && hasBaseLocal) {
      localDelta = {extraLocal.x - baseLocal.x, extraLocal.y - baseLocal.y,
                    extraLocal.z - baseLocal.z};
    }

    //==============================
    // 追加部位の「長さだけ」を反映する
    // fixed の見た目長さ × (extraCP長 / baseCP長)
    //==============================
    if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
      int basePartId = -1;
      bool hasBasePartId = GetFirstPartTypePartId(partType, basePartId);

      if (hasBasePartId) {
        int extraOwnerId = GetExtraSnapshotOwnerId(partType, partId, parentId);

        int baseParentId = -1;
        GetPartInstanceParentId(basePartId, baseParentId);

        int baseOwnerId =
            GetExtraSnapshotOwnerId(partType, basePartId, baseParentId);

        float extraSegmentLength =
            GetSnapshotSegmentLength(partType, extraOwnerId);
        float baseSegmentLength =
            GetSnapshotSegmentLength(partType, baseOwnerId);

        if (baseSegmentLength > 0.0001f) {
          float lengthRatio = extraSegmentLength / baseSegmentLength;

          obj->objectParts_[0].transform.scale =
              source->objectParts_[0].transform.scale;
          obj->objectParts_[0].transform.translate =
              source->objectParts_[0].transform.translate;

          obj->objectParts_[0].transform.scale.y *= lengthRatio;
          obj->objectParts_[0].transform.translate.y *= lengthRatio;
        }
      }
    }

    //==============================
    // 一段目（上腕）
    // 基本上腕の正しい位置 + extraとの差分
    //==============================
    if (partType == ModBodyPart::LeftUpperArm ||
        partType == ModBodyPart::RightUpperArm) {
      Object *chestBody = GetStandardPart(ModBodyPart::ChestBody);

      if (chestBody != nullptr) {
        const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
        const Vector3 &baseArmLocal = source->mainPosition.transform.translate;

        obj->mainPosition.transform.translate = {
            chestPos.x + baseArmLocal.x + localDelta.x,
            chestPos.y + baseArmLocal.y + localDelta.y,
            chestPos.z + baseArmLocal.z + localDelta.z};
      }
    }

    //==============================
    // 一段目（腿）
    // 基本腿の正しい位置 + extraとの差分
    //==============================
    if (partType == ModBodyPart::LeftThigh ||
        partType == ModBodyPart::RightThigh) {
      Object *chestBody = GetStandardPart(ModBodyPart::ChestBody);
      Object *stomachBody = GetStandardPart(ModBodyPart::StomachBody);

      if (chestBody != nullptr && stomachBody != nullptr) {
        const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
        const Vector3 &stomachLocal =
            stomachBody->mainPosition.transform.translate;
        const Vector3 &baseLegLocal = source->mainPosition.transform.translate;

        obj->mainPosition.transform.translate = {
            chestPos.x + stomachLocal.x + baseLegLocal.x + localDelta.x,
            chestPos.y + stomachLocal.y + baseLegLocal.y + localDelta.y,
            chestPos.z + stomachLocal.z + baseLegLocal.z + localDelta.z};
      }
    }

    //==============================
    // 二段目（前腕・脛）は親のアニメ後先端から生やす
    //==============================
    if (partType == ModBodyPart::LeftForeArm ||
        partType == ModBodyPart::RightForeArm ||
        partType == ModBodyPart::LeftShin ||
        partType == ModBodyPart::RightShin) {
      Object *parentObj = nullptr;
      bool hasParent = GetExtraPartParentObject(partType, parentId,
                                                extraPartObjectMap, parentObj);

      if (hasParent && parentObj != nullptr &&
          !parentObj->objectParts_.empty()) {
        const Vector3 parentRoot = parentObj->mainPosition.transform.translate;
        const float parentAngleX = parentObj->mainPosition.transform.rotate.x;
        const float parentAngleZ = parentObj->mainPosition.transform.rotate.z;
        const float parentLength =
            (std::max)(0.05f, parentObj->objectParts_[0].transform.scale.y);

        obj->mainPosition.transform.translate =
            BuildAnimatedChildRootFromParent(parentRoot, parentAngleZ,
                                             parentAngleX, parentLength);
      } else {
        // 親が取れないときだけ snapshot の Bend に逃がす
        obj->mainPosition.transform.translate = snapBend;
      }
    }

    obj->mainPosition.transform.rotate = {
        baseAngleX + ComputeExtraAnimAngleX(partType), 0.0f, baseAngleZ};

    obj->Update(camera);
  }
#endif
}

*/

Object *TravelRunner::GetStandardPart(ModBodyPart partType) const {
  int partId = -1;
  if (GetFirstPartTypePartId(partType, partId)) {
    auto it = allPartObjects_.find(partId);
    if (it != allPartObjects_.end())
      return it->second;
  }
  return nullptr;
}
