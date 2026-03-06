#pragma once
#include "BaseScene.h"

class TitleScene : public BaseScene {
public:
  TitleScene(kEngine *system);
  ~TitleScene();

  void Update() override;
  void Draw() override;

private:
  kEngine *system_ = nullptr;

  Light *light1_ = nullptr;
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;
};