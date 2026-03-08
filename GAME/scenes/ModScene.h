#pragma once
#include "../effect/Fade.h"
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
  // 仮ライト
  Light *light1_ = nullptr;

  // カメラ
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = true;

  //int modModelHandle_ = 0;
  //Object *modObject_ = nullptr;
  //ModBody modBody_{};
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> modModelHandles_{};
  std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> modObjects_{};

  Fade fade_;
  bool isStartTransition_ = false;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
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
