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
class Object;

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
	std::unique_ptr<Light> light1_;

	// カメラ
	std::weak_ptr<Camera> camera_;
	std::weak_ptr<DebugCamera> debugCamera_;
	std::weak_ptr<Camera> usingCamera_;

	bool useDebugCamera_ = false;

	// フェード
	Fade fade_;
	float GetFadeAlpha() const override { return fade_.GetAlpha(); }

	std::unique_ptr<Light> spotLight_;
	float spotLightTimer_ = 0.0f;

	Object* backgroundWall_ = nullptr;

	bool isStartTransition_ = false;

	bool wasPaused_ = false;

	// お題管理
	ThemeManager* themeManager_ = nullptr;
	ThemeData* selectedTheme_ = nullptr;

	// 審査員管理
	JudgeManager* judgeManager_ = nullptr;

	BitmapFont font_;
	std::unique_ptr<DetailButton>themeButton_;

	// 音声
	int drumrollSoundHandle_ = -1;
	int drumrollEndSoundHandle_ = -1;
	bool isEndSoundPlayed_ = false;

	// お題演出
	std::unique_ptr<PromptBoard> promptBoard_ = nullptr;
	PromptRollState rollState_ = PromptRollState::Rolling;

	// UI
	SimpleSprite* titleSprite_ = nullptr;

	int stopInputLockFrame_ = 20;
	int stopInputLockCounter_ = 0;

	int selectedPromptShowFrame_ = 20;
	int selectedPromptShowCounter_ = 0;

private:
	void CameraPart();

	void UpdatePromptRoll();
	void DecidePrompt();
};