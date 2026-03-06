#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"

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

  bool useDebugCamera_ = false;

  Fade fade_;
  bool isStartTransition_ = false;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();
};
