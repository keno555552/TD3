#pragma once
#include "BaseScene.h"

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

  bool useDebugCamera_ = false;

private:
  void CameraPart();
};
