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

class TravelScene;
class Camera;
struct ModBodyCustomizeData;
class TravelPlayer;

class TravelNpcManager {
public:
  TravelNpcManager(kEngine *system);
  ~TravelNpcManager();

  void InitializeNpcRunners(const ModBodyCustomizeData* customizeData, const TravelPlayer* player, float goalX);
  void UpdateNpcRunners(float deltaTime, float goalX, Camera* camera);
  void UpdateNpcInput(NpcRunner &npc, float deltaTime, int npcIndex);
  void UpdateNpcMovement(NpcRunner &npc, float deltaTime, int npcIndex);

  std::unique_ptr<ModBodyCustomizeData> CreateNpcPresetDefault();
  std::unique_ptr<ModBodyCustomizeData> CreateNpcPresetHeadBig();
  std::unique_ptr<ModBodyCustomizeData> CreateNpcPresetLongLeg();
  std::unique_ptr<ModBodyCustomizeData> CreateNpcPresetBigTorso();

  void SetupNpcPartObject(
      std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> &objects,
      std::array<ModBody, static_cast<size_t>(ModBodyPart::Count)> &bodies,
      ModBodyPart part, const std::string &path);

  void SetupNpcCustomizedVisual(NpcRunner &npc);
  void BuildNpcCustomizedVisual(NpcRunner &npc);
  void UpdateNpcCustomizedVisual(NpcRunner &npc, Camera* camera);
  void DrawNpcCustomizedVisual(NpcRunner &npc);
  void ClearNpcCustomizedVisual(NpcRunner &npc);
  void SimulateNpcHeadStart(NpcRunner &npc, float elapsedTime, int npcIndex, float goalX);

  std::vector<NpcRunner> npcRunners_;
  int npcModelHandle_ = 0;
  std::array<Object *, 16> npcDebugCpObjects_{};

private:
  kEngine *system_ = nullptr;
  const ModBodyCustomizeData* customizeData_ = nullptr;
  const TravelPlayer* player_ = nullptr;
};
