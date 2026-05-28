#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/Object/DetailButton/DetailButton.h"
#include "GAME/actor/TravelRunner.h"
#include "GAME/font/BitmapFont.h"
#include "GAME/manager/TravelNpcManager.h"
#include "GAME/effect/DustParticle.h"
#include <Object/Object.h>
#include <vector>

class TitleScene : public BaseScene {
public:
  TitleScene(kEngine *system);
  ~TitleScene();

  void Update() override;
  void Draw() override;

private:
  BitmapFont font_;
  std::unique_ptr<DetailButton> nextButton_;

  Object *titleTextObject_ = nullptr;
  int titleTextModelHandle_ = 0;
  float logoAnimTimer_ = 0.0f;

  enum class LogoAnimState {
    Wave,          // 基本（ウェーブ）
    DropStamp,     // 2: ドロップ＆スタンプ
    Awakening,     // 3: 覚醒・発光浮遊
    SpinJump       // 4: たまに出る（横回転）
  };
  LogoAnimState currentLogoState_ = LogoAnimState::Wave;
  float logoStateTimer_ = 0.0f;
  float logoNextStateDuration_ = 5.0f;

  std::unique_ptr<DustParticle> dust_;
  bool hasSpawnedDust_ = false;
  bool hasSpawnedDustArray_[5] = {false, false, false, false, false};

  // 仮ライト
  Light *light1_ = nullptr;

  // カメラ
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;
  Camera *titleCamera_ = nullptr; // タイトルロゴ用

  // フェード
  Fade fade_;
  float GetFadeAlpha() const override { return fade_.GetAlpha(); }
  bool isStartTransition_ = false;

  // 背景NPC演出
  std::unique_ptr<TravelRunner> titleNpcPlayer_;
  std::unique_ptr<TravelNpcManager> titleNpcManager_;
  std::unique_ptr<ModBodyCustomizeData> titleNpcDummyData_;
  int titleNpcModelHandle_ = 0;

  static constexpr float kNpcLoopLimitX = 35.0f;
  static constexpr float kNpcStartX = -20.0f;

  struct NpcLoopSetting {
    float cooldownDuration = 3.0f;
    int currentPresetId = -1;
  };
  std::vector<NpcLoopSetting> npcLoopSettings_;

  void ResetTitleNpcBody(int index);
  void ResetTitleNpc(int index);
};