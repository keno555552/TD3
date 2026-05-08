#include "TravelNpcManager.h"
#include "kEngine.h"
#include "GAME/actor/TravelPlayer.h"
#include <cmath>
#include <algorithm>

namespace {
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }
float RandomFloat(float minValue, float maxValue) {
  float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  return minValue + (maxValue - minValue) * t;
}
}
TravelNpcManager::TravelNpcManager(kEngine *system) : system_(system) {
}

TravelNpcManager::~TravelNpcManager() {
}


void TravelNpcManager::InitializeNpcRunners(const ModBodyCustomizeData* customizeData, const TravelPlayer* player, float goalX) {
  customizeData_ = customizeData;
  player_ = player;
  npcRunners_.clear();

  // まずは ModScene から来た人数を優先
  size_t npcCount = 4;
  if (customizeData_ != nullptr &&
      !customizeData_->npcStartProgressList.empty()) {
    npcCount = customizeData_->npcStartProgressList.size();
  }

  npcRunners_.resize(npcCount);

  for (size_t i = 0; i < npcRunners_.size(); ++i) {
    npcRunners_[i].moveX = -18.0f;
    npcRunners_[i].laneX = -3.0f - static_cast<float>(i) * 2.0f;

    npcRunners_[i].moveY = 1.94f;
    npcRunners_[i].velocityX = 0.0f;
    npcRunners_[i].velocityY = 0.0f;
    npcRunners_[i].leftLegBend = 0.0f;
    npcRunners_[i].rightLegBend = 0.0f;
    npcRunners_[i].leftLegBendSpeed = 0.0f;
    npcRunners_[i].rightLegBendSpeed = 0.0f;

    npcRunners_[i].bodyTilt = 0.0f;
    npcRunners_[i].bodyTiltVelocity = 0.0f;
    npcRunners_[i].landTimer = 0.0f;

    npcRunners_[i].leftInput = false;
    npcRunners_[i].rightInput = false;

    npcRunners_[i].kickThisFrame = false;
    npcRunners_[i].kickSideThisFrame = 0;

    npcRunners_[i].isGrounded = true;
    npcRunners_[i].finished = false;
    npcRunners_[i].finishRank = -1;

    npcRunners_[i].phaseTimer = 0.0f;
    npcRunners_[i].phase = static_cast<int>(i % 4);

    npcRunners_[i].inputHoldTimer = 0.0f;
    npcRunners_[i].inputLeftActive = false;
    npcRunners_[i].isKickHolding = false;
    npcRunners_[i].kickHoldLeft = false;
    npcRunners_[i].hasKickPlan = false;
    npcRunners_[i].prevGrounded = false;
    npcRunners_[i].lastTimingResult = 0;
    npcRunners_[i].timingSkill = 1.0f;
    npcRunners_[i].headStartSpeed = 2.0f * npcRunners_[i].timingSkill;

    npcRunners_[i].kickedThisAirborne = false;

    //==============================
    // ModScene から開始状態を受け取る
    //==============================
    npcRunners_[i].started = false;
    npcRunners_[i].startDelay = 0.0f;

    if (customizeData_ != nullptr &&
        i < customizeData_->npcStartProgressList.size()) {
      const auto &progress = customizeData_->npcStartProgressList[i];

      npcRunners_[i].timingSkill = progress.runTimingSkill;

      if (progress.hasStartedMoving) {
        npcRunners_[i].started = true;
        npcRunners_[i].startDelay = 0.0f;

        SimulateNpcHeadStart(npcRunners_[i], progress.moveElapsedTime, static_cast<int>(i), goalX);
      } else {
        npcRunners_[i].started = false;

        float remainTime = progress.totalTime - progress.elapsedTime;
        if (remainTime < 0.0f) {
          remainTime = 0.0f;
        }
        npcRunners_[i].startDelay = remainTime;
      }
    }

    //==============================
    // debugObject
    //==============================
    npcRunners_[i].debugObject = std::make_unique<Object>();
    npcRunners_[i].debugObject->IntObject(system_);
    npcRunners_[i].debugObject->CreateModelData(npcModelHandle_);
    npcRunners_[i].debugObject->mainPosition.transform =
        CreateDefaultTransform();
    npcRunners_[i].debugObject->mainPosition.transform.scale = {0.6f, 0.6f,
                                                                0.6f};

    //==============================
    // customized visual
    // ModScene から渡ってきた presetType を使う
    //==============================
    std::unique_ptr<ModBodyCustomizeData> preset;

    NpcPresetType presetType = NpcPresetType::Default;
    if (customizeData_ != nullptr &&
        i < customizeData_->npcStartProgressList.size()) {
      presetType = customizeData_->npcStartProgressList[i].presetType;
    }

    switch (presetType) {
    case NpcPresetType::Default:
      preset = CreateNpcPresetDefault();
      break;

    case NpcPresetType::HeadBig:
      preset = CreateNpcPresetDefault();
      preset = CreateNpcPresetHeadBig();
      break;

    case NpcPresetType::LongLeg:
      preset = CreateNpcPresetDefault();
      preset = CreateNpcPresetLongLeg();
      break;

    case NpcPresetType::BigTorso:
      preset = CreateNpcPresetDefault();
      preset = CreateNpcPresetBigTorso();
      break;

    default:
      preset = CreateNpcPresetDefault();
      break;
    }

    npcRunners_[i].useCustomizedVisual = false;
    npcRunners_[i].visualInitialized = false;
    npcRunners_[i].customizeData = std::move(preset);

    //==============================
    // 脚長から接地オフセット計算
    //==============================
    if (npcRunners_[i].customizeData != nullptr) {
      const auto &leftThigh =
          npcRunners_[i]
              .customizeData->partParams[ToIndex(ModBodyPart::LeftThigh)];
      const auto &rightThigh =
          npcRunners_[i]
              .customizeData->partParams[ToIndex(ModBodyPart::RightThigh)];
      const auto &leftShin =
          npcRunners_[i]
              .customizeData->partParams[ToIndex(ModBodyPart::LeftShin)];
      const auto &rightShin =
          npcRunners_[i]
              .customizeData->partParams[ToIndex(ModBodyPart::RightShin)];

      float avgThigh = (leftThigh.scale.y + rightThigh.scale.y) * 0.5f;
      float avgShin = (leftShin.scale.y + rightShin.scale.y) * 0.5f;

      float legScale = (avgThigh + avgShin) * 0.5f;

      npcRunners_[i].legGroundOffset = 3.5f * legScale;
    } else {
      npcRunners_[i].legGroundOffset = 3.5f;
    }

    if (npcRunners_[i].customizeData != nullptr) {
      SetupNpcCustomizedVisual(npcRunners_[i]);
    }
  }
}

void TravelNpcManager::UpdateNpcRunners(float deltaTime, float goalX, Camera* camera) {
  static float speedLogTimer = 0.0f;
  static std::array<float, 4> prevMoveX = {-18.0f, -18.0f, -18.0f, -18.0f};

  speedLogTimer += deltaTime;

  for (size_t i = 0; i < npcRunners_.size(); ++i) {
    auto &npc = npcRunners_[i];

    if (!npc.started) {
      npc.startDelay -= deltaTime;
      if (npc.startDelay <= 0.0f) {
        npc.started = true;
      }
      continue;
    }

    // if (npc.finished) {
    //   continue;
    // }

    if (npc.finished && npc.moveX > goalX + 20.0f) {
      continue;
    }

    UpdateNpcInput(npc, deltaTime, static_cast<int>(i));
    UpdateNpcMovement(npc, deltaTime, static_cast<int>(i));

    if (speedLogTimer >= 1.0f) {
      float deltaX = npc.moveX - prevMoveX[i];
      prevMoveX[i] = npc.moveX;
    }

    if (npc.moveX >= goalX) {
      npc.finished = true;
    }

    if (npc.useCustomizedVisual) {
      if (!npc.visualInitialized) {
        BuildNpcCustomizedVisual(npc);
        npc.visualInitialized = true;
      }
      UpdateNpcCustomizedVisual(npc, camera);
    } else if (npc.debugObject) {
      npc.debugObject->mainPosition.transform.translate = {npc.laneX, npc.moveY,
                                                           npc.moveX};
      npc.debugObject->Update(camera);
    }
  }

  if (speedLogTimer >= 1.0f) {
    speedLogTimer = 0.0f;
  }
}

void TravelNpcManager::UpdateNpcInput(NpcRunner &npc, float deltaTime,
                                 int npcIndex) {
  npc.leftInput = false;
  npc.rightInput = false;

  // 物理用の蹴りフラグは毎フレームリセット
  npc.kickThisFrame = false;
  npc.kickSideThisFrame = 0;

  const float perfectTimingEnd = 0.08f * 0.45f;
  const float bestTimingEnd = 0.08f;
  const float lateTimingEnd = 0.22f;

  bool justLanded = (!npc.prevGrounded && npc.isGrounded);

  // if (npc.isKickHolding || npc.leftInput || npc.rightInput) {
  // Logger::Log("NPC[%d] INPUT | hold=%d L=%d R=%d prevGround=%d
  // nowGround=%d",
  //             npcIndex, npc.isKickHolding ? 1 : 0, npc.leftInput ? 1 : 0,
  //             npc.rightInput ? 1 : 0, npc.prevGrounded ? 1 : 0,
  //             npc.isGrounded ? 1 : 0);
  //}

  //==============================
  // 着地した瞬間に今回のキック計画を作る
  // ここで「この着地中にはまだ蹴っていない」状態に戻す
  //==============================
  if (justLanded) {
    npc.isKickHolding = false;
    npc.hasKickPlan = true;
    npc.kickedThisAirborne = false;

    float skill = std::clamp(npc.timingSkill, 0.70f, 1.30f);
    float skill01 = std::clamp((skill - 0.70f) / (1.30f - 0.70f), 0.0f, 1.0f);

    float perfectWeight = 0.15f + skill01 * 0.65f;
    float goodWeight = 0.55f - skill01 * 0.10f;
    float badWeight = 1.0f - perfectWeight - goodWeight;

    perfectWeight = std::clamp(perfectWeight, 0.05f, 0.85f);
    goodWeight = std::clamp(goodWeight, 0.10f, 0.75f);
    badWeight = std::clamp(badWeight, 0.05f, 0.70f);

    float roll = RandomFloat(0.0f, 1.0f);

    if (roll < perfectWeight) {
      npc.targetKickTime = RandomFloat(0.0f, perfectTimingEnd * 0.95f);
      npc.lastTimingResult = 1;
    } else if (roll < perfectWeight + goodWeight) {
      npc.targetKickTime = RandomFloat(perfectTimingEnd, bestTimingEnd);
      npc.lastTimingResult = 2;
    } else {
      npc.targetKickTime = RandomFloat(bestTimingEnd, lateTimingEnd + 0.05f);
      npc.lastTimingResult = 3;
    }
  }

  //==============================
  // 蹴った足を空中の間だけ保持
  // 地面に触れたら保持終了
  //==============================
  if (npc.isKickHolding) {
    if (npc.kickHoldLeft) {
      npc.leftInput = true;
      npc.rightInput = false;
    } else {
      npc.leftInput = false;
      npc.rightInput = true;
    }

    // 地面に触れても少しだけ見た目保持を続ける
    if (npc.isGrounded && npc.landTimer >= 0.08f) {
      npc.isKickHolding = false;
      npc.leftInput = false;
      npc.rightInput = false;
      npc.prevGrounded = npc.isGrounded;
      return;
    }

    npc.prevGrounded = npc.isGrounded;
    return;
  }

  //==============================
  // 空中では新しい蹴りを出さない
  //==============================
  if (!npc.isGrounded) {
    npc.prevGrounded = npc.isGrounded;
    return;
  }

  //==============================
  // 初回や保険で計画が無ければ作る
  //==============================
  if (!npc.hasKickPlan) {
    npc.hasKickPlan = true;
    npc.targetKickTime = RandomFloat(perfectTimingEnd, bestTimingEnd);
    npc.lastTimingResult = 2;
  }

  //==============================
  // 1着地につき1回だけ蹴る
  //==============================
  if (!npc.kickedThisAirborne && npc.landTimer >= npc.targetKickTime) {
    npc.kickHoldLeft = !npc.kickHoldLeft;
    npc.isKickHolding = true;
    npc.hasKickPlan = false;
    npc.kickedThisAirborne = true;

    if (npc.kickHoldLeft) {
      npc.leftInput = true;
      npc.rightInput = false;
      npc.kickThisFrame = true;
      npc.kickSideThisFrame = -1;
    } else {
      npc.leftInput = false;
      npc.rightInput = true;
      npc.kickThisFrame = true;
      npc.kickSideThisFrame = 1;
    }
  }

  npc.prevGrounded = npc.isGrounded;
}

void TravelNpcManager::UpdateNpcMovement(NpcRunner &npc, float deltaTime,
                                    int npcIndex) {
  // 前フレームの接地状態を保存
  bool wasGrounded = npc.isGrounded;

  //================================
  // 脚の曲げ計算
  //================================
  float leftTargetLegAngle = npc.leftInput ? player_->legKickAngle_ : player_->legRecoverAngle_;
  float rightTargetLegAngle = npc.rightInput ? player_->legKickAngle_ : player_->legRecoverAngle_;

  npc.leftLegBendSpeed +=
      (leftTargetLegAngle - npc.leftLegBend) * player_->legFollowPower_;
  npc.rightLegBendSpeed +=
      (rightTargetLegAngle - npc.rightLegBend) * player_->legFollowPower_;

  npc.leftLegBendSpeed =
      std::clamp(npc.leftLegBendSpeed, -player_->legMaxSpeed_, player_->legMaxSpeed_);
  npc.rightLegBendSpeed =
      std::clamp(npc.rightLegBendSpeed, -player_->legMaxSpeed_, player_->legMaxSpeed_);

  npc.leftLegBendSpeed *= player_->jointDamping_;
  npc.rightLegBendSpeed *= player_->jointDamping_;

  npc.leftLegBend += npc.leftLegBendSpeed;
  npc.rightLegBend += npc.rightLegBendSpeed;

  npc.leftLegBend = std::clamp(npc.leftLegBend, -0.8f, 1.0f);
  npc.rightLegBend = std::clamp(npc.rightLegBend, -0.8f, 1.0f);

  //================================
  // 物理判定と接地更新
  // 見た目で使っている足先ワールド座標をそのまま使う
  //================================
  if (npc.moveY <= player_->groundY_ + npc.legGroundOffset) {
    npc.moveY = player_->groundY_ + npc.legGroundOffset;
    npc.velocityY = 0.0f;
    npc.isGrounded = true;
  } else {
    npc.isGrounded = false;
  }

  // 着地タイマー更新 (これが Input 側の判定タイミングになる)
  bool justLanded = (!wasGrounded && npc.isGrounded);

  // if (justLanded) {
  // Logger::Log("NPC[%d] LAND | L=%d R=%d hold=%d kickHoldLeft=%d velX=%.3f "
  //             "velY=%.3f landTimer=%.3f wasGround=%d nowGround=%d",
  //             npcIndex, npc.leftInput ? 1 : 0, npc.rightInput ? 1 : 0,
  //             npc.isKickHolding ? 1 : 0, npc.kickHoldLeft ? 1 : 0,
  //             npc.velocityX, npc.velocityY, npc.landTimer,
  //             wasGrounded ? 1 : 0, npc.isGrounded ? 1 : 0);
  //}

  if (justLanded) {
    npc.landTimer = 0.0f;
    npc.leftLegBendSpeed = 0.0f;
    npc.rightLegBendSpeed = 0.0f;
  } else if (npc.isGrounded) {
    npc.landTimer += deltaTime;
  } else {
    npc.landTimer = 999.0f;
  }

  //================================
  // 個体別のチューニングパラメータ作成
  //================================
  float runPower = 1.0f;
  float lift = 1.0f;
  float maxSpeed = 1.0f;
  float headScale = 1.0f;
  float avgTorsoScale = 1.0f;

  if (npc.customizeData != nullptr) {
    const auto &chest =
        npc.customizeData->partParams[ToIndex(ModBodyPart::ChestBody)];
    const auto &stomach =
        npc.customizeData->partParams[ToIndex(ModBodyPart::StomachBody)];
    const auto &head =
        npc.customizeData->partParams[ToIndex(ModBodyPart::Head)];

    avgTorsoScale = (chest.scale.x + chest.scale.y + chest.scale.z +
                     stomach.scale.x + stomach.scale.y + stomach.scale.z) /
                    6.0f;
    headScale = (head.scale.x + head.scale.y + head.scale.z) / 3.0f;

    runPower -= (headScale - 1.0f) * 0.6f;
    lift -= (headScale - 1.0f) * 0.4f;
    maxSpeed -= (headScale - 1.0f) * 0.5f;
    maxSpeed -= (avgTorsoScale - 1.0f) * 0.1f;

    runPower = std::clamp(runPower, 0.4f, 2.5f);
    lift = std::clamp(lift, 0.4f, 2.5f);
    maxSpeed = std::clamp(maxSpeed, 0.4f, 3.0f);
  }

  //================================
  // 姿勢（bodyTilt）の更新
  //================================
  const float npcIdealRunTilt = -0.12f;
  const float npcNeutralTilt = -0.03f;
  const float npcRecoverStartTilt = 0.42f;
  const float npcRecoverTargetTilt = -0.12f;

  float headRecoveryPenalty =
      std::clamp(1.0f - (headScale - 1.0f) * 0.55f, 0.25f, 1.0f);
  float torsoStabilityScale =
      std::clamp(1.0f + (avgTorsoScale - 1.0f) * 3.0f, 0.45f, 4.2f);
  float torsoTiltResistance =
      std::clamp(1.0f - (avgTorsoScale - 1.0f) * 1.8f, 0.10f, 1.15f);

  float neutralReturnPower = 0.002f * headRecoveryPenalty * torsoStabilityScale;
  if (!npc.isGrounded) {
    neutralReturnPower *= 0.35f;
  }

  // 中立への戻り
  npc.bodyTiltVelocity += (npcNeutralTilt - npc.bodyTilt) * neutralReturnPower;

  //================================
  // 蹴り（加速と姿勢変化）
  //================================
  bool doKick = npc.isGrounded && npc.kickThisFrame;

  if (doKick) {
    float avgLegScaleY = 1.0f;
    if (npc.customizeData != nullptr) {
      const auto &leftThigh =
          npc.customizeData->partParams[ToIndex(ModBodyPart::LeftThigh)];
      const auto &rightThigh =
          npc.customizeData->partParams[ToIndex(ModBodyPart::RightThigh)];
      const auto &leftShin =
          npc.customizeData->partParams[ToIndex(ModBodyPart::LeftShin)];
      const auto &rightShin =
          npc.customizeData->partParams[ToIndex(ModBodyPart::RightShin)];
      avgLegScaleY = (leftThigh.scale.y + rightThigh.scale.y +
                      leftShin.scale.y + rightShin.scale.y) *
                     0.25f;
    }

    float legLengthScale =
        std::clamp(1.0f + (avgLegScaleY - 1.0f) * 0.45f, 0.70f, 2.0f);
    float kickLegBend = 0.0f;
    if (npc.kickSideThisFrame == -1) {
      kickLegBend = npc.leftLegBend;
    } else if (npc.kickSideThisFrame == 1) {
      kickLegBend = npc.rightLegBend;
    }
    float kickLegForwardness =
        1.0f - std::clamp((kickLegBend - player_->legKickAngle_) /
                              (player_->legRecoverAngle_ - player_->legKickAngle_),
                          0.0f, 1.0f);
    float kickEfficiency = 0.75f + kickLegForwardness * 0.75f;
    float groundBoost = 1.2f;

    float totalPush = 0.09f;

    // landTimer に基づいた判定結果（インパルス）
    const float bestTimingEnd = 0.08f;
    const float lateTimingEnd = 0.22f;
    const float perfectTimingEnd = bestTimingEnd * 0.45f;
    float tiltImpulse = 0.0f;

    if (npc.landTimer <= perfectTimingEnd) {
      tiltImpulse = -0.055f;
      npc.lastTimingResult = 1; // Perfect
    } else if (npc.landTimer <= bestTimingEnd) {
      float goodRatio = (npc.landTimer - perfectTimingEnd) /
                        (bestTimingEnd - perfectTimingEnd);
      tiltImpulse = -0.035f + std::clamp(goodRatio, 0.0f, 1.0f) * 0.015f;
      npc.lastTimingResult = 2; // Good
    } else {
      float badRatio =
          (npc.landTimer - bestTimingEnd) / (lateTimingEnd - bestTimingEnd);
      tiltImpulse = 0.020f + std::clamp(badRatio, 0.0f, 1.0f) * 0.035f;
      npc.lastTimingResult = 3; // Bad
    }

    float headHeavyFactor =
        std::clamp(1.0f + (headScale - 1.0f) * 1.6f, 1.0f, 2.4f);
    float turnResponse = std::clamp(1.0f + (headScale - 1.0f) * 1.2f -
                                        (avgTorsoScale - 1.0f) * 0.5f,
                                    0.7f, 2.5f);
    float npcBaseTiltResponse = 2.2f;

    // 姿勢に回転力を与える
    npc.bodyTiltVelocity += tiltImpulse * npcBaseTiltResponse *
                            headHeavyFactor * torsoTiltResistance;

    // 加速計算
    float pushMagnitude = totalPush *
                          (0.55f + runPower * 1.10f + lift * 0.75f) *
                          legLengthScale * kickEfficiency * groundBoost;
    float tiltDiff = npc.bodyTilt - npcIdealRunTilt;
    float forwardRatio = std::clamp(0.5f - tiltDiff * 0.25f, 0.15f, 1.50f);
    float upwardRatio = std::clamp(0.5f + tiltDiff * 0.25f, 0.15f, 1.50f);

    if (tiltDiff > 0.0f) {
      float backwardPenalty = std::clamp(tiltDiff / 0.22f, 0.0f, 1.0f);
      forwardRatio *= (1.0f - backwardPenalty * 0.55f);
    }

    float dirLen =
        std::sqrt(forwardRatio * forwardRatio + upwardRatio * upwardRatio);
    if (dirLen > 0.0001f) {
      forwardRatio /= dirLen;
      upwardRatio /= dirLen;
    }

    npc.velocityX += pushMagnitude * forwardRatio;
    npc.velocityY += pushMagnitude * upwardRatio;

    // Logger::Log("NPC[%d] PUSH | kick=%d side=%d hold=%d kickHoldLeft=%d "
    //             "velX=%.3f velY=%.3f landTimer=%.3f",
    //             npcIndex, npc.kickThisFrame ? 1 : 0, npc.kickSideThisFrame,
    //             npc.isKickHolding ? 1 : 0, npc.kickHoldLeft ? 1 : 0,
    //             npc.velocityX, npc.velocityY, npc.landTimer);
  }

  //================================
  // 速度と姿勢の最終更新
  //================================
  float tiltDamping =
      std::clamp(0.82f + (avgTorsoScale - 1.0f) * 0.08f, 0.76f, 0.92f);
  npc.bodyTiltVelocity *= tiltDamping;

  float headTiltRangeFactor =
      std::clamp(1.0f + (headScale - 1.0f) * 1.60f, 1.0f, 3.20f);
  float torsoTiltRangeFactor =
      std::clamp(1.0f - (avgTorsoScale - 1.0f) * 0.45f, 0.65f, 1.10f);

  npc.bodyTilt += npc.bodyTiltVelocity;
  npc.bodyTilt =
      std::clamp(npc.bodyTilt,
                 player_->maxForwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor,
                 player_->maxBackwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor);

  // 起き上がり処理
  if (std::abs(npc.bodyTilt) > npcRecoverStartTilt) {
    float recoverPower = 0.010f * headRecoveryPenalty * torsoStabilityScale;
    if (std::abs(npc.bodyTilt) > 0.65f) { // 重度の転倒
      recoverPower *= 1.4f;
      npc.velocityX *= 0.96f;
    }
    npc.bodyTiltVelocity +=
        (npcRecoverTargetTilt - npc.bodyTilt) * recoverPower;
  }

  // 移動更新
  npc.velocityX *= player_->inertia_;
  npc.velocityX = std::clamp(npc.velocityX, -1.2f * maxSpeed, 1.2f * maxSpeed);

  npc.moveX += npc.velocityX;
  npc.velocityY -= player_->gravity_;
  npc.moveY += npc.velocityY;
}

std::unique_ptr<ModBodyCustomizeData> TravelNpcManager::CreateNpcPresetDefault() {
  std::unique_ptr<ModBodyCustomizeData> data;

  if (customizeData_ != nullptr) {
    data = std::make_unique<ModBodyCustomizeData>(*customizeData_);
  } else {
    data = ModBody::CreateDefaultCustomizeData();
  }

  if (data == nullptr) {
    return nullptr;
  }

  // Logger::Log("NPC PRESET DEFAULT partInstances=%d snapshots=%d",
  //             (int)data->partInstances.size(),
  //             (int)data->controlPointSnapshots.size());

  // for (const auto &inst : data->partInstances) {
  //   Logger::Log("NPC PRESET partType=%d partId=%d parentId=%d",
  //               (int)inst.partType, inst.partId, inst.parentId);
  // }

  return data;
}

std::unique_ptr<ModBodyCustomizeData> TravelNpcManager::CreateNpcPresetHeadBig() {
  std::unique_ptr<ModBodyCustomizeData> data = CreateNpcPresetDefault();

  if (data == nullptr) {
    return nullptr;
  }

  auto &headParam = data->partParams[ToIndex(ModBodyPart::Head)];
  headParam.scale.x *= 2.0f;
  headParam.scale.y *= 2.0f;
  headParam.scale.z *= 2.0f;

  auto &neckParam = data->partParams[ToIndex(ModBodyPart::Neck)];
  neckParam.scale.x *= 1.15f;
  neckParam.scale.z *= 1.15f;

  return data;
}

std::unique_ptr<ModBodyCustomizeData> TravelNpcManager::CreateNpcPresetLongLeg() {
  std::unique_ptr<ModBodyCustomizeData> data = CreateNpcPresetDefault();

  if (data == nullptr) {
    return nullptr;
  }

  // 太もも
  auto &leftThigh = data->partParams[ToIndex(ModBodyPart::LeftThigh)];
  auto &rightThigh = data->partParams[ToIndex(ModBodyPart::RightThigh)];

  leftThigh.scale.y *= 2.0f;
  rightThigh.scale.y *= 2.0f;

  // 脛
  auto &leftShin = data->partParams[ToIndex(ModBodyPart::LeftShin)];
  auto &rightShin = data->partParams[ToIndex(ModBodyPart::RightShin)];

  leftShin.scale.y *= 2.0f;
  rightShin.scale.y *= 2.0f;

  return data;
}

std::unique_ptr<ModBodyCustomizeData> TravelNpcManager::CreateNpcPresetBigTorso() {
  std::unique_ptr<ModBodyCustomizeData> data = CreateNpcPresetDefault();

  if (data == nullptr) {
    return nullptr;
  }

  auto &chest = data->partParams[ToIndex(ModBodyPart::ChestBody)];
  auto &stomach = data->partParams[ToIndex(ModBodyPart::StomachBody)];

  chest.scale.x *= 2.0f;
  chest.scale.y *= 2.0f;
  chest.scale.z *= 2.0f;

  stomach.scale.x *= 2.0f;
  stomach.scale.y *= 2.0f;
  stomach.scale.z *= 2.0f;

  return data;
}

void TravelNpcManager::SetupNpcCustomizedVisual(NpcRunner &npc) {
  npc.modObjects.fill(nullptr);

  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::ChestBody,
                     "GAME/resources/modBody/chest/chest.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::StomachBody,
                     "GAME/resources/modBody/stomach/stomach.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::Neck,
                     "GAME/resources/modBody/neck/neck.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::Head,
                     "GAME/resources/modBody/head/head.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftUpperArm,
                     "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftForeArm,
                     "GAME/resources/modBody/leftForeArm/leftForeArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightUpperArm,
                     "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightForeArm,
                     "GAME/resources/modBody/rightForeArm/rightForeArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftThigh,
                     "GAME/resources/modBody/leftThighs/leftThighs.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftShin,
                     "GAME/resources/modBody/leftShin/leftShin.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightThigh,
                     "GAME/resources/modBody/rightThighs/rightThighs.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightShin,
                     "GAME/resources/modBody/rightShin/rightShin.obj");

  if (npc.customizeData == nullptr) {
    return;
  }

  for (size_t i = 0; i < npc.modBodies.size(); ++i) {
    npc.modBodies[i].SetParam(npc.customizeData->partParams[i]);
  }

  npc.useCustomizedVisual = true;

  Object *chestBody = npc.modObjects[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = npc.modObjects[ToIndex(ModBodyPart::StomachBody)];
  Object *leftUpperArm = npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *rightUpperArm = npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)];
  Object *leftThigh = npc.modObjects[ToIndex(ModBodyPart::LeftThigh)];
  Object *rightThigh = npc.modObjects[ToIndex(ModBodyPart::RightThigh)];

  if (chestBody == nullptr || stomachBody == nullptr ||
      leftUpperArm == nullptr || rightUpperArm == nullptr ||
      leftThigh == nullptr || rightThigh == nullptr) {
    return;
  }

  ObjectPart *chestRoot = &chestBody->mainPosition;
  ObjectPart *stomachRoot = &stomachBody->mainPosition;
  ObjectPart *leftUpperArmRoot = &leftUpperArm->mainPosition;
  ObjectPart *rightUpperArmRoot = &rightUpperArm->mainPosition;
  ObjectPart *leftThighRoot = &leftThigh->mainPosition;
  ObjectPart *rightThighRoot = &rightThigh->mainPosition;

  stomachBody->followObject_ = chestRoot;
  stomachBody->mainPosition.parentPart = chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::Neck)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::Neck)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::Head)]->followObject_ =
      &npc.modObjects[ToIndex(ModBodyPart::Neck)]->mainPosition;
  npc.modObjects[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart =
      &npc.modObjects[ToIndex(ModBodyPart::Neck)]->mainPosition;

  npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)]->followObject_ =
      chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightForeArm)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightForeArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::LeftThigh)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftThigh)]->mainPosition.parentPart =
      stomachRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightThigh)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightThigh)]->mainPosition.parentPart =
      stomachRoot;

  npc.modObjects[ToIndex(ModBodyPart::LeftShin)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftShin)]->mainPosition.parentPart =
      stomachRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightShin)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightShin)]->mainPosition.parentPart =
      stomachRoot;
}

void TravelNpcManager::BuildNpcCustomizedVisual(NpcRunner &npc) {
  for (size_t i = 0; i < npc.modObjects.size(); ++i) {
    if (npc.modObjects[i] != nullptr) {
      npc.modBodies[i].Apply(npc.modObjects[i]);
    }
  }
}

void TravelNpcManager::UpdateNpcCustomizedVisual(NpcRunner &npc, Camera* camera) {
  Object *chestBody = npc.modObjects[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = npc.modObjects[ToIndex(ModBodyPart::StomachBody)];
  Object *neck = npc.modObjects[ToIndex(ModBodyPart::Neck)];
  Object *head = npc.modObjects[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = npc.modObjects[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = npc.modObjects[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = npc.modObjects[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = npc.modObjects[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = npc.modObjects[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = npc.modObjects[ToIndex(ModBodyPart::RightShin)];

  if (chestBody == nullptr || stomachBody == nullptr || neck == nullptr ||
      head == nullptr || leftUpperArm == nullptr || leftForeArm == nullptr ||
      rightUpperArm == nullptr || rightForeArm == nullptr ||
      leftThigh == nullptr || leftShin == nullptr || rightThigh == nullptr ||
      rightShin == nullptr) {
    return;
  }

  auto SafePartLength = [](Object *obj) -> float {
    if (obj == nullptr || obj->objectParts_.empty()) {
      return 1.0f;
    }
    return (std::max)(0.05f, obj->objectParts_[0].transform.scale.y);
  };

  auto BuildAnimatedChildRoot = [](const Vector3 &root, float angleX,
                                   float length) -> Vector3 {
    return {root.x, root.y - std::cos(angleX) * length,
            root.z - std::sin(angleX) * length};
  };

  const float chestLength = SafePartLength(chestBody);
  const float stomachLength = SafePartLength(stomachBody);
  const float neckLength = SafePartLength(neck);

  const float leftUpperArmLength = SafePartLength(leftUpperArm);
  const float leftForeArmLength = SafePartLength(leftForeArm) * 2.0f;
  const float rightUpperArmLength = SafePartLength(rightUpperArm);
  const float rightForeArmLength = SafePartLength(rightForeArm) * 2.0f;

  const float leftThighLength = SafePartLength(leftThigh) * 2.0f;
  const float leftShinLength = SafePartLength(leftShin) * 2.0f;
  const float rightThighLength = SafePartLength(rightThigh) * 2.0f;
  const float rightShinLength = SafePartLength(rightShin) * 2.0f;

  //==============================
  // bend の中間を基準に使う
  // これをしないと「初期姿勢だけ強い」になりやすい
  //==============================
  const float legAnimCenter = (player_->legKickAngle_ + player_->legRecoverAngle_) * 0.5f;

  float leftLegAnim = npc.leftLegBend - legAnimCenter;
  float rightLegAnim = npc.rightLegBend - legAnimCenter;

  const float armSwingScale = 1.4f;
  const float thighSwingScale = 1.4f;

  const float bodyTiltArmAssist = npc.bodyTilt * 0.35f;

  const float leftUpperArmRotX =
      -rightLegAnim * armSwingScale + bodyTiltArmAssist;
  const float rightUpperArmRotX =
      -leftLegAnim * armSwingScale + bodyTiltArmAssist;

  const float leftThighRotX = -leftLegAnim * thighSwingScale;
  const float rightThighRotX = -rightLegAnim * thighSwingScale;

  //==============================
  // 胴体スケール取得
  //==============================
  float chestWidthScale = 1.0f;
  float chestHeightScale = 1.0f;
  float stomachWidthScale = 1.0f;
  float stomachHeightScale = 1.0f;
  float neckHeightScale = 1.0f;

  if (npc.customizeData != nullptr) {
    const auto &chestParam =
        npc.customizeData->partParams[ToIndex(ModBodyPart::ChestBody)];
    const auto &stomachParam =
        npc.customizeData->partParams[ToIndex(ModBodyPart::StomachBody)];
    const auto &neckParam =
        npc.customizeData->partParams[ToIndex(ModBodyPart::Neck)];

    chestWidthScale = chestParam.scale.x;
    chestHeightScale = chestParam.scale.y;
    stomachWidthScale = stomachParam.scale.x;
    stomachHeightScale = stomachParam.scale.y;
    neckHeightScale = neckParam.scale.y;
  }

  //==============================
  // 胸
  //==============================
  chestBody->mainPosition.transform.translate.x = npc.laneX;
  chestBody->mainPosition.transform.translate.y = npc.moveY;
  chestBody->mainPosition.transform.translate.z = npc.moveX;
  chestBody->mainPosition.transform.rotate = {-npc.bodyTilt, 0.0f, 0.0f};

  if (!chestBody->objectParts_.empty()) {
    chestBody->objectParts_[0].transform.translate = {0.0f, chestLength * 0.5f,
                                                      0.0f};
    float torsoTotalHeight = (chestLength + stomachLength) * 0.8f;
    chestBody->mainPosition.transform.translate.y =
        npc.moveY + torsoTotalHeight;
  }

  //==============================
  // 腹
  //==============================
  stomachBody->mainPosition.transform.translate = {0.0f, -chestLength * 2.0f,
                                                   0.0f};
  stomachBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  if (!stomachBody->objectParts_.empty()) {
    stomachBody->objectParts_[0].transform.translate = {0.0f, 1.5f, 0.0f};

    float chestToStomachOffset = (chestLength + stomachLength) * 0.4f;
    stomachBody->mainPosition.transform.translate = {
        0.0f, -chestToStomachOffset, 0.0f};
  }

  //==============================
  // 首・頭
  //==============================
  const Vector3 neckBaseLocal = {0.0f, chestLength * 0.5f, 0.0f};

  neck->mainPosition.transform.translate = neckBaseLocal;
  neck->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  head->mainPosition.transform.translate = {0.0f, neckLength * 0.05f, 0.0f};
  head->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  //==============================
  // 肩位置
  //==============================
  const Vector3 leftShoulderLocal = {
      -1.5f * chestWidthScale, chestLength * 0.32f * chestHeightScale, 0.0f};

  const Vector3 rightShoulderLocal = {
      1.5f * chestWidthScale, chestLength * 0.32f * chestHeightScale, 0.0f};

  //==============================
  // 股関節位置
  //==============================
  const Vector3 leftHipLocal = {-0.5f * stomachWidthScale,
                                -stomachLength * 0.28f * stomachHeightScale,
                                0.0f};

  const Vector3 rightHipLocal = {0.5f * stomachWidthScale,
                                 -stomachLength * 0.28f * stomachHeightScale,
                                 0.0f};

  //==============================
  // 擬似CP
  // 肩→肘、股関節→膝を毎フレーム作る
  //==============================
  Vector3 leftElbowLocal = BuildAnimatedChildRoot(
      leftShoulderLocal, leftUpperArmRotX, leftUpperArmLength);

  Vector3 rightElbowLocal = BuildAnimatedChildRoot(
      rightShoulderLocal, rightUpperArmRotX, rightUpperArmLength);

  Vector3 leftKneeLocal =
      BuildAnimatedChildRoot(leftHipLocal, leftThighRotX, leftThighLength);

  Vector3 rightKneeLocal =
      BuildAnimatedChildRoot(rightHipLocal, rightThighRotX, rightThighLength);

  Vector3 leftHandLocal = BuildAnimatedChildRoot(
      leftElbowLocal, leftUpperArmRotX, leftForeArmLength);

  Vector3 rightHandLocal = BuildAnimatedChildRoot(
      rightElbowLocal, rightUpperArmRotX, rightForeArmLength);

  Vector3 leftFootLocal =
      BuildAnimatedChildRoot(leftKneeLocal, leftThighRotX, leftShinLength);

  Vector3 rightFootLocal =
      BuildAnimatedChildRoot(rightKneeLocal, rightThighRotX, rightShinLength);

  auto Add = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
  };

  auto SetNpcDebugMarker = [&](int index, const Vector3 &worldPos) {
    if (index < 0 || index >= static_cast<int>(npcDebugCpObjects_.size())) {
      return;
    }

    Object *obj = npcDebugCpObjects_[index];
    if (obj == nullptr) {
      return;
    }

    obj->mainPosition.transform.translate = worldPos;
    obj->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
    obj->Update(camera);
  };

  const Vector3 chestWorld = chestBody->mainPosition.transform.translate;
  const Vector3 stomachWorld =
      Add(chestWorld, stomachBody->mainPosition.transform.translate);

  const Vector3 neckBaseWorld = Add(chestWorld, neckBaseLocal);
  const Vector3 headAnchorWorld =
      Add(neckBaseWorld, head->mainPosition.transform.translate);

  const Vector3 leftShoulderWorld = Add(chestWorld, leftShoulderLocal);
  const Vector3 rightShoulderWorld = Add(chestWorld, rightShoulderLocal);

  const Vector3 leftElbowWorld = Add(chestWorld, leftElbowLocal);
  const Vector3 rightElbowWorld = Add(chestWorld, rightElbowLocal);

  const Vector3 leftHandWorld = Add(chestWorld, leftHandLocal);
  const Vector3 rightHandWorld = Add(chestWorld, rightHandLocal);

  const Vector3 leftHipWorld = Add(stomachWorld, leftHipLocal);
  const Vector3 rightHipWorld = Add(stomachWorld, rightHipLocal);

  const Vector3 leftKneeWorld = Add(stomachWorld, leftKneeLocal);
  const Vector3 rightKneeWorld = Add(stomachWorld, rightKneeLocal);

  const Vector3 leftFootWorld = Add(stomachWorld, leftFootLocal);
  const Vector3 rightFootWorld = Add(stomachWorld, rightFootLocal);

  SetNpcDebugMarker(0, neckBaseWorld);
  SetNpcDebugMarker(1, headAnchorWorld);

  SetNpcDebugMarker(2, leftShoulderWorld);
  SetNpcDebugMarker(3, leftElbowWorld);
  SetNpcDebugMarker(4, leftHandWorld);

  SetNpcDebugMarker(5, rightShoulderWorld);
  SetNpcDebugMarker(6, rightElbowWorld);
  SetNpcDebugMarker(7, rightHandWorld);

  SetNpcDebugMarker(8, leftHipWorld);
  SetNpcDebugMarker(9, leftKneeWorld);
  SetNpcDebugMarker(10, leftFootWorld);

  SetNpcDebugMarker(11, rightHipWorld);
  SetNpcDebugMarker(12, rightKneeWorld);
  SetNpcDebugMarker(13, rightFootWorld);

  SetNpcDebugMarker(14, chestWorld);
  SetNpcDebugMarker(15, stomachWorld);

  //==============================
  // 上腕
  //==============================
  leftUpperArm->mainPosition.transform.translate = leftShoulderLocal;
  leftUpperArm->mainPosition.transform.rotate = {leftUpperArmRotX, 0.0f, 0.0f};

  rightUpperArm->mainPosition.transform.translate = rightShoulderLocal;
  rightUpperArm->mainPosition.transform.rotate = {rightUpperArmRotX, 0.0f,
                                                  0.0f};

  if (!leftUpperArm->objectParts_.empty()) {
    leftUpperArm->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    //  leftUpperArm->objectParts_[0].transform.scale = {1.0f, 0.8f, 1.0f};
  }

  if (!rightUpperArm->objectParts_.empty()) {
    rightUpperArm->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    //   rightUpperArm->objectParts_[0].transform.scale = {1.0f, 0.8f, 1.0f};
  }

  //==============================
  // 前腕
  //==============================
  leftForeArm->mainPosition.transform.translate = leftElbowLocal;
  leftForeArm->mainPosition.transform.rotate = {leftUpperArmRotX * 0.45f, 0.0f,
                                                0.0f};

  rightForeArm->mainPosition.transform.translate = rightElbowLocal;
  rightForeArm->mainPosition.transform.rotate = {rightUpperArmRotX * 0.45f,
                                                 0.0f, 0.0f};

  if (!leftForeArm->objectParts_.empty()) {
    leftForeArm->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    //  leftForeArm->objectParts_[0].transform.scale = {1.0f, 0.5f, 1.0f};
  }

  if (!rightForeArm->objectParts_.empty()) {
    rightForeArm->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    //   rightForeArm->objectParts_[0].transform.scale = {1.0f, 0.5f, 1.0f};
  }

  //==============================
  // 腿
  //==============================
  leftThigh->mainPosition.transform.translate = leftHipLocal;
  leftThigh->mainPosition.transform.rotate = {leftThighRotX, 0.0f, 0.0f};

  rightThigh->mainPosition.transform.translate = rightHipLocal;
  rightThigh->mainPosition.transform.rotate = {rightThighRotX, 0.0f, 0.0f};

  if (!leftThigh->objectParts_.empty()) {
    leftThigh->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    //  leftThigh->objectParts_[0].transform.scale = {1.0f, 0.5f, 1.0f};
  }

  if (!rightThigh->objectParts_.empty()) {
    rightThigh->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    //  rightThigh->objectParts_[0].transform.scale = {1.0f, 0.5f, 1.0f};
  }

  //==============================
  // 脛
  //==============================
  leftShin->mainPosition.transform.translate = leftKneeLocal;
  leftShin->mainPosition.transform.rotate = {leftThighRotX * 0.55f, 0.0f, 0.0f};

  rightShin->mainPosition.transform.translate = rightKneeLocal;
  rightShin->mainPosition.transform.rotate = {rightThighRotX * 0.55f, 0.0f,
                                              0.0f};

  if (!leftShin->objectParts_.empty()) {
    leftShin->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    // leftShin->objectParts_[0].transform.scale = {1.0f, 0.5f, 1.0f};
  }

  if (!rightShin->objectParts_.empty()) {
    rightShin->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
    // rightShin->objectParts_[0].transform.scale = {1.0f, 0.5f, 1.0f};
  }

  auto SafePartRadius = [](Object *obj) -> float {
    if (obj == nullptr || obj->objectParts_.empty()) {
      return 0.1f;
    }
    return (std::max)(0.01f, obj->objectParts_[0].transform.scale.x * 0.5f);
  };

  // 当たり判定用
  npc.leftFootWorld = leftFootWorld;
  npc.rightFootWorld = rightFootWorld;

  // static int npcVisualLogFrame = 0;
  // npcVisualLogFrame++;

  // if ((npcVisualLogFrame % 20) == 0) {
  //   Logger::Log("NPC VISUAL FOOT       | moveX=%.3f moveY=%.3f "
  //               "leftFoot=(%.3f, %.3f, %.3f) rightFoot=(%.3f, %.3f, %.3f)",
  //               npc.moveX, npc.moveY, npc.leftFootWorld.x,
  //               npc.leftFootWorld.y, npc.leftFootWorld.z,
  //               npc.rightFootWorld.x, npc.rightFootWorld.y,
  //               npc.rightFootWorld.z);
  // }

  //==============================
  // 反映
  //==============================
  for (size_t i = 0; i < npc.modObjects.size(); ++i) {
    if (npc.modObjects[i] != nullptr) {
      npc.modObjects[i]->Update(camera);
    }
  }
}

void TravelNpcManager::DrawNpcCustomizedVisual(NpcRunner &npc) {
  for (auto *obj : npc.modObjects) {
    if (obj != nullptr) {
      obj->Draw();
    }
  }

  for (auto *obj : npc.extraObjects) {
    if (obj != nullptr) {
      obj->Draw();
    }
  }
}

void TravelNpcManager::ClearNpcCustomizedVisual(NpcRunner &npc) {
  for (auto &obj : npc.modObjects) {
    delete obj;
    obj = nullptr;
  }

  for (auto *obj : npc.extraObjects) {
    delete obj;
  }
  npc.extraObjects.clear();

  npc.useCustomizedVisual = false;
}

void TravelNpcManager::SimulateNpcHeadStart(NpcRunner &npc, float elapsedTime, int npcIndex, float goalX) {
  const float simDeltaTime = 1.0f / 60.0f;
  float remain = elapsedTime;

  while (remain > 0.0f) {
    float dt = (remain < simDeltaTime) ? remain : simDeltaTime;

    UpdateNpcInput(npc, dt, npcIndex);
    UpdateNpcMovement(npc, dt, npcIndex);

    static int npcHeadStartLogFrame = 0;
    npcHeadStartLogFrame++;

    if ((npcHeadStartLogFrame % 20) == 0) {
      // Logger::Log("NPC[%d] HEADSTART | moveX=%.3f moveY=%.3f leftFootY=%.3f "
      //             "rightFootY=%.3f visualInit=%d useCustom=%d",
      //             npcIndex, npc.moveX, npc.moveY, npc.leftFootWorld.y,
      //             npc.rightFootWorld.y, npc.visualInitialized ? 1 : 0,
      //             npc.useCustomizedVisual ? 1 : 0);
    }

    if (npc.moveX >= goalX) {
      npc.finished = true;
      break;
    }

    remain -= dt;
  }
}

void TravelNpcManager::SetupNpcPartObject(
    std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> &objects,
    std::array<ModBody, static_cast<size_t>(ModBodyPart::Count)> &bodies,
    ModBodyPart part, const std::string &path) {
  const size_t index = static_cast<size_t>(part);

  int modelHandle = system_->SetModelObj(path);

  objects[index] = new Object;
  objects[index]->IntObject(system_);
  objects[index]->CreateModelData(modelHandle);
  
  Transform t;
  t.translate = {0,0,0};
  t.rotate = {0,0,0};
  t.scale = {1,1,1};
  objects[index]->mainPosition.transform = t;

  bodies[index].Initialize(objects[index], part);
}