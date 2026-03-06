#pragma once
#include "BaseScene.h"

class ContestScene : public BaseScene {
public:
  ContestScene(kEngine *system);
  ~ContestScene();

  void Update() override;
  void Draw() override;

private:
  kEngine *system_ = nullptr;

  Light *light1_ = nullptr;

  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = false;

private:
  void CameraPart();
};
