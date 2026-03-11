#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <array>
#include <memory>
#include <string>

class ModScene : public BaseScene {
public:
  ModScene(kEngine *system);
  ~ModScene();

  void Update() override;
  void Draw() override;

private:
  // 仮ライト
  Light *light1_ = nullptr;

  // カメラ
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = true;

  std::array<int, static_cast<size_t>(ModBodyPart::Count)> modModelHandles_{};
  std::array<std::unique_ptr<Object>, static_cast<size_t>(ModBodyPart::Count)>
      modObjects_{};
  std::array<ModBody, static_cast<size_t>(ModBodyPart::Count)> modBodies_{};

  // Body root 基準の各 joint 位置
  std::array<Vector3, static_cast<size_t>(ModBodyPart::Count)>
      bodyJointOffsets_{};

  std::unique_ptr<ModBodyCustomizeData> customizeData_ = nullptr;

  Fade fade_;
  bool isStartTransition_ = false;

private:
  void CameraPart();

  void SetupModObjects();
  void SetupPartObject(ModBodyPart part, const std::string &path);
  void SetupHierarchy();
  void SetupInitialLayout();
  void SetupBodyJointOffsets();

  // 共有改造データから部位へ値を反映する
  void LoadCustomizeData();
  // 部位から共有改造データへ値を反映する
  void SyncCustomizeDataFromScene();

  void UpdateChildRootsFromBody();

  void ApplyModBodies();
  void ResetModBodies();

  void UpdateModObjects();
  void DrawModObjects();

#ifdef USE_IMGUI
  void DrawModGui();
#endif
};