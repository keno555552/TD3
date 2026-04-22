#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include <Object/Object.h>
#include"GAME/Object/DetailButton/DetailButton.h"
#include "GAME/font/BitmapFont.h"

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

  // フェード
  Fade fade_;
  bool isStartTransition_ = false;
};