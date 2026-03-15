#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"

// お題
#include "GAME/theme/ThemeManager.h"
#include "GAME/theme/ThemeData.h"

class PromptScene : public BaseScene {
public:
  PromptScene(kEngine *system);
  ~PromptScene();

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

  // フェード
  Fade fade_;
  bool isStartTransition_ = false;

  // お題選出
  ThemeManager* themeManager_ = nullptr;
  ThemeData* selectedTheme_ = nullptr;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();
};
