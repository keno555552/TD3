#include "CountdownTimer.h"

CountdownTimer::CountdownTimer(kEngine* kEngine) {
	SimpleSprite::IntObject(kEngine);
	timer_ = std::make_unique<Timer>();
	timer_->InitM(maxTime_, kEngine->GetTimeManager());

	lessTimeColor = { 255.0f / 255.0f,36.0f / 255.0f,29.0f / 255.0f ,255.0f / 255.0f };

	/// ========= テキスチャーハンドル =========///

	std::string basePath = "resources/texture/CountdownTimer/";

	numberTextureHandles_ = kEngine->LoadTexture(basePath + "Iwata_Antic_Std_number.png");
	pointTextureHandle_ = kEngine->LoadTexture(basePath + "Iwata_Antic_Std_dot.png");
	leftTimeTextureHandle_ = kEngine->LoadTexture(basePath + "Ink_Free_Regular_leftTime.png");

	/// ============ オブジェクト =============///

	mainPosition.transform.translate = { 640.0f,50.0f,0.0f };

	leftTimeSprite_ = std::make_unique<SimpleSprite>();
	leftTimeSprite_->IntObject(kEngine);
	leftTimeSprite_->CreateDefaultData();
	leftTimeSprite_->mainPosition.transform.scale = { scale_,scale_,1.0f };
	leftTimeSprite_->objectParts_[0].materialConfig->textureHandle = leftTimeTextureHandle_;
	leftTimeSprite_->objectParts_[0].anchorPoint = leftTimeSpriteSize / 2;
	leftTimeSprite_->followObject_ = this;

	numberSprites_ = std::make_unique <SimpleSprite>();
	numberSprites_->IntObject(kEngine);
	NumberSpritesInit();
	numberSprites_->followObject_ = this;

	pointSprite_ = std::make_unique <SimpleSprite>();
	pointSprite_->IntObject(kEngine);
	pointSprite_->CreateDefaultData();
	pointSprite_->mainPosition.transform.scale = { scale_,scale_,1.0f };
	pointSprite_->objectParts_[0].materialConfig->textureHandle = pointTextureHandle_;
	pointSprite_->objectParts_[0].anchorPoint = pointSpriteSize / 2;
	pointSprite_->followObject_ = this;

	PositionSetting();
}

void CountdownTimer::NumberSpritesInit() {
	float sizeX = numberSpriteSize.x / 10.0f * 4 + pointSpriteSize.x;
	float cropW = 114.0f;
	float cropH = 160.0f;

	Vector2 startPos = { -sizeX / 2.0f ,0.0f };
	for (int i = 0; i < 4; i++) {
		float dotSize = 0;
		if (i > 1)dotSize = pointSpriteSize.x;
		numberSprites_->CreateDefaultData();
		numberSprites_->objectParts_.back().materialConfig->textureHandle = numberTextureHandles_;
		numberSprites_->objectParts_.back().anchorPoint = { numberSpriteSize.x / 20.0f,numberSpriteSize.y / 2.0f };
		numberSprites_->objectParts_.back().transform.translate = { startPos.x + (cropW)*i + dotSize,startPos.y,0.0f };
		numberSprites_->objectParts_.back().transform.scale = { scale_ / 10.0f,scale_,1.0f };
		numberSprites_->objectParts_.back().cropSize = Vector2(cropW, cropH);
	}
}

void CountdownTimer::PositionSetting() {

	/// ================== 全体配置 ================== ///

	Vector2 middlePoint = { 0.0f,0.0f };
	float numberSpriteSizeX = (numberSpriteSize.x / 10.0f * 4 + pointSpriteSize.x) * scale_;
	float leftTimeSpriteSizeX = leftTimeSpriteSize.x * scale_;
	float holdSizeX = numberSpriteSizeX + leftTimeSpriteSizeX;

	float startX = middlePoint.x - holdSizeX / 2;

	leftTimeSprite_->mainPosition.transform.translate.x = startX + leftTimeSpriteSizeX / 2;
	leftTimeSprite_->mainPosition.transform.translate.y = middlePoint.y;

	numberSprites_->mainPosition.transform.translate.x = startX + leftTimeSpriteSizeX + (numberSpriteSize.x / 10.0f * 2) * scale_ + (pointSpriteSize.x / 2) * scale_;
	numberSprites_->mainPosition.transform.translate.y = middlePoint.y;

	pointSprite_->mainPosition.transform.translate.x = numberSprites_->mainPosition.transform.translate.x;
	pointSprite_->mainPosition.transform.translate.y = middlePoint.y;

	leftTimeSprite_->mainPosition.transform.scale = { scale_,scale_,1.0f };
	pointSprite_->mainPosition.transform.scale = { scale_,scale_,1.0f };

	/// ================== 数字配置 ================== ///

	float sizeX = (numberSpriteSize.x / 10.0f * 4 + pointSpriteSize.x) * scale_;
	float cropW = 114.0f * scale_;
	float cropH = 160.0f * scale_;

	Vector2 startPos = { -sizeX / 2.0f ,0.0f };
	for (int i = 0; i < 4; i++) {
		float dotSize = 0;
		if (i > 1)dotSize = (pointSpriteSize.x) * scale_;
		numberSprites_->objectParts_[i].transform.translate = { startPos.x + (cropW)*i + dotSize,startPos.y,0.0f };
		numberSprites_->objectParts_[i].transform.scale = { scale_ / 10.0f,scale_,1.0f };
	}

}

CountdownTimer::~CountdownTimer() {

	numberSprites_.reset();
	pointSprite_.reset();
	leftTimeSprite_.reset();

	timer_.reset();
}

void CountdownTimer::Update(Camera* camera) {

	/// ================= オブジェクト更新 ================= ///
	PositionSetting();

	/// ============== タイマー更新 ============== ///
	if (isStart_)timer_->ToZero();

	/// ================== 数字分解 ================== ///
	timeArray.clear();

	float t = timer_->parameter_;
	if (t <= 0.0f) t = 0.0f;
	if (t >= maxTime_) t = maxTime_;

	// 秒（整數部分）
	int seconds = int(t);

	// 小數部分（兩位）
	int fraction = int((t - seconds) * 100.0f); // 0~99

	// 拆成 S1 S2 F1 F2
	int s1 = seconds / 10;
	int s2 = seconds % 10;

	int f1 = fraction / 10;
	int f2 = fraction % 10;

	// 填入 timeArray（不含小數點）
	timeArray.push_back(f2);
	timeArray.push_back(f1);
	timeArray.push_back(s2);
	timeArray.push_back(s1);

	/// ============== スプライトの内容調整 ================ ///

	bool isChangeColor = (timer_->parameter_ <= lessTime_) && ((int)(timer_->parameter_ * 2) % 2 == 1);

	for (int i = 0; i < 4; i++) {
		// 設定裁切
		float cropW = 114.0f;
		int digit = timeArray[4 - 1 - i] - 1;
		numberSprites_->objectParts_[i].cropLT = Vector2(digit * cropW, 0.0f);

		// 色変更
		if (isChangeColor) {
			numberSprites_->objectParts_[i].materialConfig->textureColor = lessTimeColor;
		} else {
			numberSprites_->objectParts_[i].materialConfig->textureColor = { 1.0f,1.0f,1.0f,1.0f };
		}
	}
	// 色変更
	if (isChangeColor) {
		pointSprite_->objectParts_[0].materialConfig->textureColor = lessTimeColor;
	} else {
		pointSprite_->objectParts_[0].materialConfig->textureColor = { 1.0f,1.0f,1.0f,1.0f };
	}

	/// ============== スプライト数調整 ================ ///

}

void CountdownTimer::Draw() {
	leftTimeSprite_->Draw();
	numberSprites_->Draw();
	pointSprite_->Draw();
}

void CountdownTimer::Start() {
	isStart_ = true;
}

void CountdownTimer::Stop() {
	isStart_ = false;
}

void CountdownTimer::Reset() {
	timer_->ResetM();
}

bool CountdownTimer::IsTimeUp() {
	if (timer_->parameter_ <= 0.0f)return true;
	return false;
}

bool CountdownTimer::IsLessTime() {
	if (timer_->parameter_ <= lessTime_)return true;
	return false;
}

float CountdownTimer::SetScale(float scale) {
	if (scale == -99999)return scale_;
	return 0.0f;
}

void CountdownTimer::SetLessTimeColor(Vector4 color) {
	lessTimeColor = color;
}

Timer& CountdownTimer::GetTime() {
	return *timer_;
}

