#pragma once
#include "BaseScene.h"
#include "Object/Object.h"
#include "GAME/actor/ModBody.h"
#include <array>

class ModScene : public BaseScene {
public:
  ModScene(kEngine *system);
  ~ModScene();

  void Update() override;
  void Draw() override;

private:
  kEngine *system_ = nullptr;

  Light *light1_ = nullptr;

  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = true;

  //int modModelHandle_ = 0;
  //Object *modObject_ = nullptr;
  //ModBody modBody_{};
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> modModelHandles_{};
  std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> modObjects_{};

private:
  void CameraPart();
  //void SetupModObject();
  void SetupModObjects();
  void SetupPartObject(ModBodyPart part, const std::string &path);
  void SetupHierarchy();
  void SetupInitialLayout();
  void UpdateModObjects();
  void DrawModObjects();

#ifdef USE_IMGUI
  void DrawModGui();
#endif
};
