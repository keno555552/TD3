#pragma once
#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <array>
#include <memory>
#include <vector>

class kEngine;

struct NpcRunner {
  float moveX = -18.0f;
  float moveY = 0.0f;
  float velocityX = 0.0f;
  float velocityY = 0.0f;

  float leftLegBend = 0.0f;
  float rightLegBend = 0.0f;
  float leftLegBendSpeed = 0.0f;
  float rightLegBendSpeed = 0.0f;

  float bodyTilt = 0.0f;
  float bodyTiltVelocity = 0.0f;

  float landTimer = 999.0f;

  bool leftInput = false;
  bool rightInput = false;
  bool isGrounded = false;
  bool finished = false;

  float phaseTimer = 0.0f;
  int phase = 0; // 0:蟾ｦ謚ｼ縺・1:蟾ｦ髮｢縺・2:蜿ｳ謚ｼ縺・3:蜿ｳ髮｢縺・
  float laneX = 0.0f;

  float startDelay = 0.0f;
  bool started = false;
  float headStartSpeed = 2.0f;

  float inputHoldTimer = 0.0f;
  bool inputLeftActive = false;
  float baseKickInterval = 0.25f;
  bool isKickHolding = false;
  bool kickHoldLeft = false;

  float timingSkill = 1.0f;
  float targetKickTime = 0.05f;
  bool hasKickPlan = false;
  bool prevGrounded = false;

  int lastTimingResult = 0;

  int finishRank = -1;

  std::unique_ptr<Object> debugObject;

  bool useCustomizedVisual = false;

  std::unique_ptr<ModBodyCustomizeData> customizeData;

  std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> modObjects{};
  std::array<ModBody, static_cast<size_t>(ModBodyPart::Count)> modBodies{};

  std::vector<Object *> extraObjects;

  bool visualInitialized = false;

  Vector3 leftFootWorld = {};
  Vector3 rightFootWorld = {};
  float leftFootRadius = 1.0f;
  float rightFootRadius = 1.0f;

  bool kickThisFrame = false;
  int kickSideThisFrame = 0; // -1:蟾ｦ, 1:蜿ｳ, 0:縺ｪ縺・
  bool kickedThisAirborne = false;

  float legGroundOffset = 5.0f;
};

class TravelNpcManager {
public:
  TravelNpcManager(kEngine *system);
  ~TravelNpcManager();

  // Variables transferred from TravelScene
  std::vector<NpcRunner> npcRunners_;
  int npcModelHandle_ = 0;

  // We will migrate NPC functions here progressively
private:
  kEngine *system_ = nullptr;
};
