#pragma once
#include <memory>
#include "Object/Object.h"
#include "Object/Sprite.h"

class CountdownTimer : public SimpleSprite
{
public:
	CountdownTimer(kEngine* kEngine);
	~CountdownTimer();

	void Update(Camera* camera) override;
	void Draw() override;
	
	void Start();
	void Stop();
	void Reset();
	bool IsTimeUp();
	bool IsLessTime();
	
	float SetScale(float scale = -99999);
	void SetLessTimeColor(Vector4 color);
	Timer& GetTime();
	

private:

	/// ========= テキスチャーハンドル =========///
	/// 
	/// 0 =1,1=2,2=3
	int numberTextureHandles_;
	int pointTextureHandle_;
	int leftTimeTextureHandle_;


	/// ============ オブジェクト =============///

	SimpleSprite* numberSprites_{};
	SimpleSprite* pointSprite_{};
	SimpleSprite* leftTimeSprite_{};

	/// =========== フラク、内部変数 ===========///
	Vector4 lessTimeColor = { 1.0f,0.0f,0.0f,1.0f };
	std::unique_ptr<Timer> timer_;
	float maxTime_ = 15.0f;
	float lessTime_ = 10.0f;
	bool isStart_ = false;
	
	/// 文字関連
	Vector2 leftTimeSpriteSize = { 571.0f,160.0f };
	Vector2 pointSpriteSize = { 54.0f,160.0f };
	Vector2 numberSpriteSize = { 1140.0f,160.0f };
	float numberSpacing = 0.2f;
	float scale_ = 0.4f;

	/// ============= フラグ ============= ///

	std::vector<int> timeArray;

private:
	/// ============= シーン内命令 =============///

	void NumberSpritesInit();
	void PositionSetting();
};