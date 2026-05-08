#include "TravelPlayer.h"
#include "kEngine.h"
#include <algorithm>
#include <cmath>

TravelPlayer::TravelPlayer(kEngine *system) : system_(system) {
}

TravelPlayer::~TravelPlayer() {
}

void TravelPlayer::Initialize(float startX) {
  moveX_ = startX;
}

void TravelPlayer::UpdateHoldState(bool leftNowInput, bool rightNowInput,
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

void TravelPlayer::UpdateLegBendState(bool leftNowInput, bool rightNowInput) {
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

void TravelPlayer::UpdateMovementState(bool leftNowInput, bool rightNowInput) {
  // NOTE: Movement state calculation is temporarily delegated from TravelScene
  // Some dependencies like GetControlPoints() or customizeData_ are still in TravelScene.
  // We will migrate those in Phase 2.
}

void TravelPlayer::ApplyVisualState() {
  // NOTE: Visual state calculation is delegated from TravelScene.
  // ModBody array is still in TravelScene.
  // We will migrate those in Phase 2.
}
