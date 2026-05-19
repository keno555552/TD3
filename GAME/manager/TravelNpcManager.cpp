#include "TravelNpcManager.h"
#include "kEngine.h"
#include "GAME/actor/TravelRunner.h"
#include "GAME/actor/ModCustomizeDataStore.h"
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


void TravelNpcManager::InitializeNpcRunners(const ModBodyCustomizeData* customizeData, const TravelRunner* player, float goalX) {
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
    npcRunners_[i].runner = std::make_unique<TravelRunner>(system_);
    npcRunners_[i].runner->Initialize(-18.0f);
    npcRunners_[i].runner->SetLaneX(-3.0f - static_cast<float>(i) * 2.0f);
    npcRunners_[i].runner->SetMoveY(1.94f);

    npcRunners_[i].laneX = -3.0f - static_cast<float>(i) * 2.0f;

    npcRunners_[i].leftInput = false;
    npcRunners_[i].rightInput = false;

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
    // customized visual
    // ModScene から渡ってきた presetType を使う
    //==============================
    std::unique_ptr<ModBodyCustomizeData> preset;

    NpcPresetType presetType = NpcPresetType::Default;
    if (customizeData_ != nullptr &&
        i < customizeData_->npcStartProgressList.size()) {
      presetType = customizeData_->npcStartProgressList[i].presetType;
    }

    if (presetType == NpcPresetType::Default) {
      preset = ModCustomizeDataStore::CreateNpcPreset(NpcPresetType::Default, customizeData_);
    } else {
      preset = ModCustomizeDataStore::CreateNpcPreset(presetType, customizeData_);
    }

    npcRunners_[i].customizeData = std::move(preset);

    if (npcRunners_[i].customizeData != nullptr) {
      npcRunners_[i].runner->SetCustomizeData(npcRunners_[i].customizeData.get());
    }
    npcRunners_[i].runner->LoadCustomizeData();
    npcRunners_[i].runner->BuildFeaturesFromCustomizeData();
    npcRunners_[i].runner->BuildAllVisualParts();
    npcRunners_[i].runner->ApplyCustomizeToMovementParam();
  }
}

void TravelNpcManager::UpdateNpcRunners(float deltaTime, float goalX, Camera* camera) {
  static float speedLogTimer = 0.0f;
  static std::vector<float> prevMoveX;
  if (prevMoveX.size() < npcRunners_.size()) {
      prevMoveX.resize(npcRunners_.size(), -18.0f);
  }
  speedLogTimer += deltaTime;

  for (size_t i = 0; i < npcRunners_.size(); ++i) {
    auto &npc = npcRunners_[i];

    if (!npc.started) {
      npc.startDelay -= deltaTime;
      if (npc.startDelay > 0.0f) {
        continue;
      }
      npc.started = true;
    }

    // if (npc.finished) {
    //   continue;
    // }

    if (npc.finished && npc.runner->GetMoveX() > goalX + 20.0f) {
      continue;
    }

    UpdateNpcInput(npc, deltaTime, static_cast<int>(i));

    if (npc.runner) {
      npc.runner->UpdateHoldState(npc.leftInput, npc.rightInput, deltaTime);
      npc.runner->UpdateLegBendState(npc.leftInput, npc.rightInput);
      npc.runner->UpdateMovementState(npc.leftInput, npc.rightInput);

    }

    if (speedLogTimer >= 1.0f) {
      float deltaX = npc.runner->GetMoveX() - prevMoveX[i];
      prevMoveX[i] = npc.runner->GetMoveX();
    }

    if (npc.runner->GetMoveX() >= goalX) {
      npc.finished = true;
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
  bool kickThisFrame = false;
  int kickSideThisFrame = 0;

  bool isGrounded = npc.runner->GetIsGrounded();
  float landTimer = npc.runner->GetLandTimer();

  const float perfectTimingEnd = 0.08f * 0.45f;
  const float bestTimingEnd = 0.08f;
  const float lateTimingEnd = 0.22f;

  bool justLanded = (!npc.prevGrounded && isGrounded);

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
    if (isGrounded && landTimer >= 0.08f) {
      npc.isKickHolding = false;
      npc.leftInput = false;
      npc.rightInput = false;
      npc.prevGrounded = isGrounded;
      return;
    }

    npc.prevGrounded = isGrounded;
    return;
  }

  //==============================
  // 空中では新しい蹴りを出さない
  //==============================
  if (!isGrounded) {
    npc.prevGrounded = isGrounded;
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
  if (!npc.kickedThisAirborne && landTimer >= npc.targetKickTime) {
    npc.kickHoldLeft = !npc.kickHoldLeft;
    npc.isKickHolding = true;
    npc.hasKickPlan = false;
    npc.kickedThisAirborne = true;

    if (npc.kickHoldLeft) {
      npc.leftInput = true;
      npc.rightInput = false;
      kickThisFrame = true;
      kickSideThisFrame = -1;
    } else {
      npc.leftInput = false;
      npc.rightInput = true;
      kickThisFrame = true;
      kickSideThisFrame = 1;
    }
  }

  npc.prevGrounded = isGrounded;
}


void TravelNpcManager::SimulateNpcHeadStart(NpcRunner &npc, float elapsedTime, int npcIndex, float goalX) {
  const float simDeltaTime = 1.0f / 60.0f;
  float remain = elapsedTime;

  while (remain > 0.0f) {
    float dt = (remain < simDeltaTime) ? remain : simDeltaTime;

    UpdateNpcInput(npc, dt, npcIndex);
    if (npc.runner) {
      npc.runner->UpdateHoldState(npc.leftInput, npc.rightInput, dt);
      npc.runner->UpdateLegBendState(npc.leftInput, npc.rightInput);
      npc.runner->UpdateMovementState(npc.leftInput, npc.rightInput);
    }

    if (npc.runner && npc.runner->GetMoveX() >= goalX) {
      npc.finished = true;
      break;
    }

    remain -= dt;
  }
}
void TravelNpcManager::DrawNpcs(float goalX, bool showNpcModel, Camera* camera) {
  for (auto &npc : npcRunners_) {
    if (!npc.started) {
      continue;
    }

    if (!showNpcModel) {
      continue;
    }

    if (npc.finished && npc.runner && npc.runner->GetMoveX() > goalX + 20.0f) {
      continue;
    }

    if (npc.runner) {
      npc.runner->ApplyVisualState();
      npc.runner->ResolveVisualGroundPenetration();
      npc.runner->UpdateModObjects();
      npc.runner->DrawModObjects(camera);
    }
  }
}
