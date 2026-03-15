#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include <memory>
#include <string>
#include <vector>

class PromptBoard;

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
  enum class PromptRollState { Rolling, Stopped, FadeOut };

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
  // お題関連
  std::unique_ptr<PromptBoard> promptBoard_ = nullptr;
  std::vector<std::string> prompts_;
  std::vector<std::string> promptTexturePaths_;
  std::vector<int> promptTextureHandles_;
  PromptRollState rollState_ = PromptRollState::Rolling;

  int currentPromptIndex_ = 0;
  int selectedPromptIndex_ = -1;

  int rollFrameCounter_ = 0;
  int rollIntervalFrame_ = 2;

  int stopInputLockFrame_ = 20;
  int stopInputLockCounter_ = 0;

  int selectedPromptShowFrame_ = 20;
  int selectedPromptShowCounter_ = 0;

private:
  void CameraPart();

  void InitializePrompts();
  void UpdatePromptRoll();
  void DecidePrompt();

  const std::string &GetCurrentPrompt() const;
};