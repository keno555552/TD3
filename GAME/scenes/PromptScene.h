#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/theme/ThemeManager.h"
#include "GAME/theme/ThemeData.h"
#include "GAME/judges/JudgeManager.h"
#include"GAME/Object/DetailButton/DetailButton.h"
#include "GAME/font/BitmapFont.h"
#include <memory>

class PromptBoard;

class PromptScene : public BaseScene {
public:
	PromptScene(kEngine* system);
	~PromptScene();

	void Update() override;
	void Draw() override;

private:
	enum class PromptRollState { Rolling, Stopped, FadeOut };

private:
	// 仮ライト
	Light* light1_ = nullptr;

	// カメラ
	Camera* camera_ = nullptr;
	DebugCamera* debugCamera_ = nullptr;
	Camera* usingCamera_ = nullptr;

	bool useDebugCamera_ = false;

	// フェード
	Fade fade_;
	bool isStartTransition_ = false;

	// お題管理
	ThemeManager* themeManager_ = nullptr;
	ThemeData* selectedTheme_ = nullptr;

	// 審査員管理
	JudgeManager* judgeManager_ = nullptr;

	BitmapFont font_;
	std::unique_ptr<DetailButton>themeButton_;

	// お題演出
	std::unique_ptr<PromptBoard> promptBoard_ = nullptr;
	PromptRollState rollState_ = PromptRollState::Rolling;

	int stopInputLockFrame_ = 20;
	int stopInputLockCounter_ = 0;

	int selectedPromptShowFrame_ = 20;
	int selectedPromptShowCounter_ = 0;

private:
	void CameraPart();

	void UpdatePromptRoll();
	void DecidePrompt();
};