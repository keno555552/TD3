#pragma once

#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <array>
#include <memory>

class kEngine;

class TravelPlayer {
public:
  TravelPlayer(kEngine* system);
  ~TravelPlayer();

  void Initialize(float startX);

  void UpdateHoldState(bool leftNowInput, bool rightNowInput, float deltaTime);
  void UpdateLegBendState(bool leftNowInput, bool rightNowInput);
  void UpdateMovementState(bool leftNowInput, bool rightNowInput);
  void ApplyVisualState();

  // Getters & Setters
  float GetMoveX() const { return moveX_; }
  void SetMoveX(float x) { moveX_ = x; }

  float GetMoveY() const { return moveY_; }
  void SetMoveY(float y) { moveY_ = y; }

  float GetVelocityX() const { return velocityX_; }
  float GetVelocityY() const { return velocityY_; }

  float GetBodyTilt() const { return bodyTilt_; }
  float GetLeftLegBend() const { return leftLegBend_; }
  float GetRightLegBend() const { return rightLegBend_; }

  float GetLeftDriveAccum() const { return leftDriveAccum_; }
  float GetRightDriveAccum() const { return rightDriveAccum_; }

  float GetLeftHoldTime() const { return leftHoldTime_; }
  float GetRightHoldTime() const { return rightHoldTime_; }

  float GetBodyTiltVelocity() const { return bodyTiltVelocity_; }
  float GetLeftLegBendSpeed() const { return leftLegBendSpeed_; }
  float GetRightLegBendSpeed() const { return rightLegBendSpeed_; }

  void SetIsGrounded(bool isGrounded) { isGrounded_ = isGrounded; }

  // Variables transferred from TravelScene
  float moveX_ = 0.0f;
  float velocityX_ = 0.0f;
  float inertia_ = 0.96f; 

  float moveY_ = 0.0f;
  float velocityY_ = 0.0f;
  float gravity_ = 0.0045f;
  float groundY_ = 0.0f;
  bool isGrounded_ = true;
  float jumpRatio_ = 3.0f;

  float bodyTilt_ = 0.0f;
  float bodyTiltVelocity_ = 0.0f;

  float maxForwardTilt_ = -0.60f;
  float maxBackwardTilt_ = 0.20f;
  float legDiffTiltPower_ = 0.08f;

  float legKickAngle_ = -0.55f;
  float legRecoverAngle_ = 0.85f;
  float legFollowPower_ = 0.18f;
  float legMaxSpeed_ = 0.12f;

  float leftLegBend_ = 0.0f;
  float rightLegBend_ = 0.0f;
  float leftLegBendSpeed_ = 0.0f;
  float rightLegBendSpeed_ = 0.0f;

  float bodyStretch_ = 0.0f;
  float bodyStretchSpeed_ = 0.0f;

  float jointDamping_ = 0.88f;

  float leftLegPrevBend_ = 0.0f;
  float rightLegPrevBend_ = 0.0f;

  float pushPower_ = 0.25f;
  float groundAssist_ = 1.0f;

  bool debugForceTilt_ = false;
  float debugTiltValue_ = 0.0f;

  float minKickPower_ = 0.45f;

  float idealRunTilt_ = -0.12f;
  float postureTolerance_ = 0.32f;
  float pushTiltKick_ = -0.2f;

  float footContactBendThreshold_ = 0.15f;
  float groundedKickFactor_ = 1.0f;

  float leftDriveAccum_ = 0.0f;
  float rightDriveAccum_ = 0.0f;

  int lastKickSide_ = 0;

  float driveBuildScale_ = 2.8f;
  float driveDecay_ = 0.01f;
  float minDriveToKick_ = 0.035f;

  float alternateKickBonus_ = 1.18f;
  float sameSideKickPenalty_ = 0.88f;

  float maxDriveAccum_ = 0.22f;
  float bothHoldBuildPenalty_ = 0.75f;

  float leftHoldTime_ = 0.0f;
  float rightHoldTime_ = 0.0f;

  float minHoldTimeToKick_ = 0.08f;
  float driveUsePerFrame_ = 0.06f;
  float drivePushScale_ = 3.0f;

  float leftLegPrevBendSpeed_ = 0.0f;
  float rightLegPrevBendSpeed_ = 0.0f;

  struct TravelTuning {
    float runPower = 1.0f;
    float maxSpeed = 1.0f;
    float stability = 1.0f;
    float lift = 1.0f;
    float turnResponse = 1.0f;
    float strideScale = 1.0f;
  };
  TravelTuning tuning_;
  bool useCustomizeMove_ = true;

  bool requireReleaseAfterLandLeft_ = false;
  bool requireReleaseAfterLandRight_ = false;

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

  int perfectStreak_ = 0;

private:
  kEngine* system_ = nullptr;
};
