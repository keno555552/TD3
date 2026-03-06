#pragma once
#include "BaseScene.h"

class TravelScene : public BaseScene {
public:
  TravelScene(kEngine *system);
  ~TravelScene();

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