#pragma once
#include "BaseScene.h"
#include "Object/Sprite.h"
#include "../effect/Fade.h"
#include <memory>

class SplashScene : public BaseScene {
public:
	SplashScene(kEngine* system);
	~SplashScene() override;

	void Update() override;
	void Draw() override;

private:
	std::unique_ptr<SimpleSprite> splashSprite_;
	int splashTextureHandle_ = -1;
	
	Fade fade_;
	float displayTimer_ = 0.0f;
	const float kDisplayDuration_ = 3.0f; // 3 seconds before auto-fade
	bool isFadingOut_ = false;

	Camera* camera_ = nullptr;
	Light* dummyLight_ = nullptr;
};
