#pragma once
#include "Object/Object.h"
#include "Object/Sprite.h"

class StartTimer : public SimpleSprite
{
public:
	StartTimer(kEngine* kEngine);
	~StartTimer();

	void Update(Camera* camera) override;
	void Draw() override;

	void Start();
	void Stop();
	void Reset();
	bool IsTimeUp();

	Timer& GetTime();
	int GetCount() const { return numberCount_; }


private:

	/// ========= テキスチャーハンドル =========///
	/// 
	/// 0 =1,1=2,2=3
	int numberTextureHandles_[3];
	int startTextureHandle_;


	/// ============ オブジェクト =============///

	std::unique_ptr<SimpleSprite> numberSprites_[3]{};
	std::unique_ptr<SimpleSprite> startSprite_ = nullptr;



	/// =========== フラク、内部変数 ===========///
	Vector4 usingColor = { 1.0f,1.0f,1.0f,0.0f };
	float startScale_ = 0.5f;
	std::unique_ptr<Timer> timer_ = nullptr;
	int numberCount_ = 3;
	bool isStart_ = false;

private:

	void ImGuiPart();

};

