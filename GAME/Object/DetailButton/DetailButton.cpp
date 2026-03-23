#include "DetailButton.h"
#

DetailButton::DetailButton(kEngine* system) {
	system_ = system;

	/// テクスチャーハンドルを保存
	texture_[0] = system_->LoadTexture("./GAME/resources/texture/button/1.png");
	texture_[1] = system_->LoadTexture("./GAME/resources/texture/button/2.png");
	texture_[2] = system_->LoadTexture("./GAME/resources/texture/button/3.png");
	texture_[3] = system_->LoadTexture("./GAME/resources/texture/button/4.png");
	texture_[4] = system_->LoadTexture("./GAME/resources/texture/button/5.png");
	texture_[5] = system_->LoadTexture("./GAME/resources/texture/button/6.png");
	texture_[6] = system_->LoadTexture("./GAME/resources/texture/button/7.png");
	texture_[7] = system_->LoadTexture("./GAME/resources/texture/button/8.png");
	texture_[8] = system_->LoadTexture("./GAME/resources/texture/button/9.png");

	/// デフォルトサイズ
	spriteDefaultSize_[0] = { 33.0f,38.0f };
	spriteDefaultSize_[1] = { 10.0f,38.0f };
	spriteDefaultSize_[2] = { 33.0f,38.0f };
	spriteDefaultSize_[3] = { 33.0f,10.0f };
	spriteDefaultSize_[4] = { 10.0f,10.0f };
	spriteDefaultSize_[5] = { 33.0f,10.0f };
	spriteDefaultSize_[6] = { 33.0f,38.0f };
	spriteDefaultSize_[7] = { 10.0f,38.0f };
	spriteDefaultSize_[8] = { 33.0f,38.0f };

	/// 倍率変化
	float scale = 0.5f;
	for (auto& size : spriteDefaultSize_) {
		size = size * scale;
	}

	/// 状態初期化
	buttonState_ = NORMAL;

	/// 色設定
	setColor_[NORMAL] = { 0.22f, 0.28f, 0.36f, 1.0f };
	setColor_[PRESS] = { 0.16f, 0.20f, 0.26f, 1.0f };
	setColor_[SELECT] = { 0.30f, 0.45f, 0.85f, 1.0f };
	setColor_[LOCK]  = { 0.45f, 0.48f, 0.52f, 1.0f };

	/// スプライトの初期化
	int i = 0;
	for (auto& ptr : sprite_) {
		ptr = std::make_unique<SimpleSprite>();
		ptr->IntObject(system_);
		ptr->CreateDefaultData();
		ptr->objectParts_[0].materialConfig->textureHandle = texture_[i];
		ptr->objectParts_[0].materialConfig->textureColor = setColor_[NORMAL];
		ptr->followObject_ = this;
		i++;
	}
}

DetailButton::~DetailButton() {
}

void DetailButton::LoadTexture(kEngine* system) {
	system->LoadTexture("./GAME/resources/texture/button/1.png");
	system->LoadTexture("./GAME/resources/texture/button/2.png");
	system->LoadTexture("./GAME/resources/texture/button/3.png");
	system->LoadTexture("./GAME/resources/texture/button/4.png");
	system->LoadTexture("./GAME/resources/texture/button/5.png");
	system->LoadTexture("./GAME/resources/texture/button/6.png");
	system->LoadTexture("./GAME/resources/texture/button/7.png");
	system->LoadTexture("./GAME/resources/texture/button/8.png");
	system->LoadTexture("./GAME/resources/texture/button/9.png");
}

void DetailButton::SetWidth(float width) {
	buttonSize_.x = width;
}

void DetailButton::SetHeight(float height) {
	buttonSize_.y = height;
}


bool DetailButton::GetIsSelect(Vector2 TPos, float width, float height) {
	if (buttonState_ == LOCK) return false;

	Vector2 A[2] = { { mainPosition.transform.translate.x - buttonSize_.x / 2, mainPosition.transform.translate.y + buttonSize_.y / 2},
					 { mainPosition.transform.translate.x + buttonSize_.x / 2, mainPosition.transform.translate.y - buttonSize_.y / 2 } };
	Vector2 B[2] = { { TPos.x - width / 2, TPos.y + height / 2},{ TPos.x + width / 2, TPos.y - height / 2 } };
	int DFlat[2] = {};

	if (B[0].x < A[1].x) { DFlat[0]++; }
	if (B[1].x > A[0].x) { DFlat[0]++; }

	if (B[0].y > A[1].y) { DFlat[1]++; }
	if (B[1].y < A[0].y) { DFlat[1]++; }

	if (DFlat[0] == 2 && DFlat[1] == 2) {
		return true;
	} else {
		return false;
	}
}

void DetailButton::ResetButton() {
	buttonState_ = NORMAL;
}


void DetailButton::SetButton(Vector2 pos, float width, float height, float layer) {

	/// ====================== 中心点設定 ====================== ///
	mainPosition.transform.translate.x = pos.x;
	mainPosition.transform.translate.y = pos.y;

	Vector2 intPos = {};

	/// ================ Buttonのサイズを計算する =============== ///
	/// ボタンのサイズ
	buttonSize_ = buttonDefaultSize_;
	bool isWidthOver = width > buttonDefaultSize_.x;
	bool isHeightOver = height > buttonDefaultSize_.y;
	if (isWidthOver) buttonSize_.x = width;
	if (isHeightOver) buttonSize_.y = height;

	/// まずは確定のサイズを代入する
	spriteSize_[LT] = spriteDefaultSize_[LT];
	spriteSize_[RT] = spriteDefaultSize_[RT];
	spriteSize_[LB] = spriteDefaultSize_[LB];
	spriteSize_[RB] = spriteDefaultSize_[RB];

	/// 次に、辺で変わらないサイズを代入する
	/// 左
	spriteSize_[LM].x = spriteSize_[LT].x;

	/// 右
	spriteSize_[RM].x = spriteSize_[RT].x;

	/// 上
	spriteSize_[CT].y = spriteSize_[LT].y;

	/// 下
	spriteSize_[CB].y = spriteSize_[LB].y;

	/// 他のを計算する
	Vector2 centerSize;
	centerSize.x = buttonSize_.x - spriteSize_[LM].x - spriteSize_[RM].x;
	centerSize.y = buttonSize_.y - spriteSize_[CT].y - spriteSize_[CB].y;

	spriteSize_[LM].y = centerSize.y;
	spriteSize_[CM].y = centerSize.y;
	spriteSize_[RM].y = centerSize.y;

	spriteSize_[CT].x = centerSize.x;
	spriteSize_[CM].x = centerSize.x;
	spriteSize_[CB].x = centerSize.x;

	Vector2 LTInitStartPos = { intPos.x - buttonSize_.x / 2.0f, intPos.y - buttonSize_.y / 2.0f };

	float rowHeight[3] = {
		spriteSize_[LT].y,
		spriteSize_[LM].y,
		spriteSize_[LB].y
	};

	float colWidth[3] = {
		spriteSize_[LT].x,
		spriteSize_[CT].x,
		spriteSize_[RT].x
	};

	int x = 0;
	Vector2 intPos2 = LTInitStartPos;

	for (int j = 0; j < 3; j++) {
	
		intPos2.x = LTInitStartPos.x;
	
		for (int i = 0; i < 3; i++) {
	
			SetQuad(sprite_[x].get(), intPos2, spriteSize_[x]);
	
			if (layer != 0)sprite_[x]->objectParts_[0].transform.translate.z = layer;
	
			intPos2.x += (colWidth[i] - 1);
			x++;
		}
	
		intPos2.y += rowHeight[j];
	}
}

void DetailButton::Update() {

	/// ボタンの位置とサイズを設定
	SetButton({ mainPosition.transform.translate.x, mainPosition.transform.translate.y }, buttonSize_.x, buttonSize_.y, mainPosition.transform.translate.z);

	/// 当たり判定
	int mouseX = 0, mouseY = 0;
	mouseX = system_->GetMousePosX();
	mouseY = system_->GetMousePosY();

	/// ボタンの状態変換
	if (buttonState_ != LOCK) {
		if (GetIsSelect({ (float)mouseX,(float)mouseY }, 1, 1)) {
			buttonState_ = SELECT;
			if (system_->GetMouseIsPush(0)) {
				buttonState_ = PRESS;
			}
		} else {
			if (!system_->GetMouseIsPush(0)) buttonState_ = NORMAL;
		}
	}
}

void DetailButton::Render() {

	DrawButton(buttonState_);

#ifdef _DEBUG
	ImGuiPart();
#endif // _DEBUG
}

void DetailButton::DrawButton(int buttonStatic) {
	if (currentColor_ != setColor_[buttonStatic]) {
		currentColor_ = setColor_[buttonStatic];
		for (auto& ptr : sprite_) {
			ptr->objectParts_[0].materialConfig->textureColor = setColor_[buttonStatic];
		}
	}
	for (auto& ptr : sprite_) {
		ptr->Draw();
	}
}

void DetailButton::SetQuad(SimpleSprite* sprite, Vector2 lt, Vector2 size) {
	sprite->objectParts_[0].conerData = {
				 lt.x,         lt.y,
				 lt.x,lt.y + size.y,
		lt.x + size.x,lt.y + size.y,
		lt.x + size.x,		   lt.y
	};
}

#ifdef _DEBUG
void DetailButton::ImGuiPart() {

	//ImGui::Begin("DetailButton");
	//ImGui::SliderFloat2("cornerPosLT", &cornerPos_[0].x, -100, 1000);
	//ImGui::SliderFloat2("cornerPosLB", &cornerPos_[1].x, -100, 1000);
	//ImGui::SliderFloat2("cornerPosRB", &cornerPos_[2].x, -100, 1000);
	//ImGui::SliderFloat2("cornerPosRT", &cornerPos_[3].x, -100, 1000);
	//ImGui::End();
}
#endif // _DEBUG
