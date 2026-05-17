#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include <Object/Object.h>
#include"GAME/Object/DetailButton/DetailButton.h"
#include "GAME/font/BitmapFont.h"
#include "GAME/actor/TravelPlayer.h"
#include "GAME/manager/TravelNpcManager.h"
#include <vector>

class TitleScene : public BaseScene {
public:
  TitleScene(kEngine *system);
  ~TitleScene();

  void Update() override;
  void Draw() override;

private:

	BitmapFont font_;
	std::unique_ptr<DetailButton>nextButton_;

	Object* titleTextObject_ = nullptr;
	int titleTextModelHandle_ = 0;

  // 仮ライト
  Light *light1_ = nullptr;

  // カメラ
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;
  Camera *titleCamera_ = nullptr; // タイトルロゴ用（正面視点）

  // フェード
  Fade fade_;
  bool isStartTransition_ = false;

  // 背景NPC演出
  std::unique_ptr<TravelPlayer>           titleNpcPlayer_;
  std::unique_ptr<TravelNpcManager>       titleNpcManager_;
  std::unique_ptr<ModBodyCustomizeData>   titleNpcDummyData_;
  int titleNpcModelHandle_ = 0;

  static constexpr float kNpcLoopLimitX = 25.0f;
  static constexpr float kNpcStartX     = -18.0f;

  struct NpcLoopSetting {
    float cooldownDuration = 3.0f;
  };
  std::vector<NpcLoopSetting> npcLoopSettings_;

  void ResetTitleNpc(int index);
};