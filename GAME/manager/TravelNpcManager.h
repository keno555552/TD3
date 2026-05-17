#pragma once
#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <array>
#include <memory>
#include <vector>
#include "GAME/actor/TravelRunner.h"

class kEngine;
class TravelRunner;

struct NpcRunner {
  // AI State Variables
  float phaseTimer = 0.0f;
  int phase = 0; 
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
  bool kickedThisAirborne = false;

  int lastTimingResult = 0;

  int finishRank = -1;
  bool finished = false;

  // AI Output Inputs for this frame
  bool leftInput = false;
  bool rightInput = false;

  std::unique_ptr<ModBodyCustomizeData> customizeData;
  std::unique_ptr<TravelRunner> runner; // Encapsulated Actor
};

class TravelScene;
class Camera;
struct ModBodyCustomizeData;


class TravelNpcManager {
public:
  TravelNpcManager(kEngine *system);
  ~TravelNpcManager();

  void InitializeNpcRunners(const ModBodyCustomizeData* customizeData, const TravelRunner* player, float goalX);
  void UpdateNpcRunners(float deltaTime, float goalX, Camera* camera);
  void UpdateNpcInput(NpcRunner &npc, float deltaTime, int npcIndex);
  void UpdateNpcMovement(NpcRunner &npc, float deltaTime, int npcIndex);

  void SimulateNpcHeadStart(NpcRunner &npc, float elapsedTime, int npcIndex, float goalX);

  void DrawNpcs(float goalX, bool showNpcModel, Camera* camera);

  std::vector<NpcRunner> npcRunners_;
  int npcModelHandle_ = 0;
  std::array<Object *, 16> npcDebugCpObjects_{};

private:
  kEngine *system_ = nullptr;
  const ModBodyCustomizeData* customizeData_ = nullptr;
  const TravelRunner* player_ = nullptr;
};
