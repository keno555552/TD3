#include "Panel.h"
#

Panel::Panel(kEngine* system) {
	system_ = system;

	/// テクスチャーハンドルを保存
	texture_[0] = system_->LoadTexture("./GAME/resources/texture/window/1.png");
	texture_[1] = system_->LoadTexture("./GAME/resources/texture/window/2.png");
	texture_[2] = system_->LoadTexture("./GAME/resources/texture/window/3.png");
	texture_[3] = system_->LoadTexture("./GAME/resources/texture/window/4.png");
	texture_[4] = system_->LoadTexture("./GAME/resources/texture/window/5.png");
	texture_[5] = system_->LoadTexture("./GAME/resources/texture/window/6.png");
	texture_[6] = system_->LoadTexture("./GAME/resources/texture/window/7.png");
	texture_[7] = system_->LoadTexture("./GAME/resources/texture/window/8.png");
	texture_[8] = system_->LoadTexture("./GAME/resources/texture/window/9.png");

	/// デフォルトサイズ
	spriteDefaultSize_[0] = { 157.0f, 41.0f };
	spriteDefaultSize_[1] = { 2.0f, 41.0f };
	spriteDefaultSize_[2] = { 22.0f, 41.0f };
	spriteDefaultSize_[3] = { 22.0f,  2.0f };
	spriteDefaultSize_[4] = { 137.0f,  2.0f };
	spriteDefaultSize_[5] = { 22.0f,  2.0f };
	spriteDefaultSize_[6] = { 22.0f, 22.0f };
	spriteDefaultSize_[7] = { 137.0f, 22.0f };
	spriteDefaultSize_[8] = { 22.0f, 22.0f };

	/// 倍率変化
	for (auto& size : spriteDefaultSize_) {
		size = size * scale_;
	}

	/// 状態初期化
	panelState_ = START;

	/// スプライトの初期化
	int i = 0;
	for (auto& ptr : sprite_) {
		ptr = std::make_unique<SimpleSprite>();
		ptr->IntObject(system_);
		ptr->CreateDefaultData();
		ptr->objectParts_[0].materialConfig->textureHandle = texture_[i];
		ptr->followObject_ = this;
		i++;
	}

	/// タイマーの初期化
	animationTimer_.Init0(0.1f, system_->GetTimeManager());

	PanelAnimation.resize(STATIC_NUM);
	PanelAnimation[START] = [=]() { OpenPanel(); };
	PanelAnimation[CLOSE] = [=]() { ClosePanel(); };
}

Panel::~Panel() {
}

void Panel::LoadTexture(kEngine* system) {
	system->LoadTexture("./GAME/resources/texture/window/1.png");
	system->LoadTexture("./GAME/resources/texture/window/2.png");
	system->LoadTexture("./GAME/resources/texture/window/3.png");
	system->LoadTexture("./GAME/resources/texture/window/4.png");
	system->LoadTexture("./GAME/resources/texture/window/5.png");
	system->LoadTexture("./GAME/resources/texture/window/6.png");
	system->LoadTexture("./GAME/resources/texture/window/7.png");
	system->LoadTexture("./GAME/resources/texture/window/8.png");
	system->LoadTexture("./GAME/resources/texture/window/9.png");
}

void Panel::SetPos(Vector2 pos, float layer) {
	mainPosition.transform.translate.x = pos.x;
	mainPosition.transform.translate.y = pos.y;
	if (layer != 0)mainPosition.transform.translate.z = layer;
}

void Panel::SetWidth(float width) {
	buttonSize_.x = width;
}

void Panel::SetHeight(float height) {
	buttonSize_.y = height;
}

void Panel::SetScale(float scale) {
	scale_ = scale;
	for (auto& size : spriteDefaultSize_) {
		size = size * scale_;
	}
}


bool Panel::GetIsSelect(Vector2 TPos, float width, float height) {
	if (panelState_ == LOCK) return false;

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

void Panel::ResetPanel() {
	panelState_ = NORMAL;
}


void Panel::SetPanel(Vector2 pos, float width, float height, float layer) {

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
	spriteSize_[LM].x = spriteSize_[LB].x;

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

	spriteSize_[CM].x = centerSize.x;
	spriteSize_[CB].x = centerSize.x;

	/// 特別のトップのセンターを計算
	spriteSize_[CT].x = buttonSize_.x - spriteSize_[LT].x - spriteSize_[RT].x;

	Vector2 LTInitStartPos = { intPos.x - buttonSize_.x / 2.0f, intPos.y - buttonSize_.y / 2.0f };

	float rowHeight[3] = {
		spriteSize_[LT].y,
		spriteSize_[LM].y,
		spriteSize_[LB].y
	};

	float colTWidth[3] = {
		spriteSize_[LT].x,
		spriteSize_[CT].x,
		spriteSize_[RT].x
	};

	float colMBWidth[3] = {
		spriteSize_[LM].x,
		spriteSize_[CM].x,
		spriteSize_[RM].x
	};

	int x = 0;
	Vector2 intPos2 = LTInitStartPos;

	for (int j = 0; j < 3; j++) {

		intPos2.x = LTInitStartPos.x;

		for (int i = 0; i < 3; i++) {

			SetQuad(sprite_[x].get(), intPos2, spriteSize_[x]);

			if (layer != 0)sprite_[x]->objectParts_[0].transform.translate.z = layer;

			x < 3 ? intPos2.x += (colTWidth[i] - 1) : intPos2.x += (colMBWidth[i] - 1);
			x++;
		}

		intPos2.y += (rowHeight[j]);
	}
}

void Panel::Update() {


	SetPanel({ mainPosition.transform.translate.x, mainPosition.transform.translate.y }, buttonSize_.x, buttonSize_.y, mainPosition.transform.translate.z);

	/// 当たり判定
	int mouseX = 0, mouseY = 0;
	mouseX = system_->GetMousePosX();
	mouseY = system_->GetMousePosY();

	if(PanelAnimation[panelState_]){
		PanelAnimation[panelState_]();
	}
}

void Panel::Render() {
	DrawPanel(panelState_);
#ifdef DEBUG
	ImGuiPart();
#endif // DEBUG
}

void Panel::DrawPanel(int buttonStatic) {

	///　今はただ描画するだけ
	for (auto& ptr : sprite_) {
		ptr->Draw();
	}
}

void Panel::SetQuad(SimpleSprite* sprite, Vector2 lt, Vector2 size) {
	sprite->objectParts_[0].conerData = {
				 lt.x,         lt.y,
				 lt.x,lt.y + size.y,
		lt.x + size.x,lt.y + size.y,
		lt.x + size.x,		   lt.y
	};
}

void Panel::OpenPanel() {

	/// タイマー運行
	if (animationTimer_.GetIsMax()) {
		panelState_ = NORMAL;
		animationTimer_.ResetM();
	} else {
		animationTimer_.ToMix();
	}

	float t = animationTimer_.linearity();

	float scale = 0.8f + 0.2f * t;

	mainPosition.transform.scale.x = scale;
	mainPosition.transform.scale.y = scale;

	for (auto& ptr : sprite_) {
		ptr->objectParts_[0].materialConfig->textureColor.w = t;
	}
}

void Panel::ClosePanel() {

	/// タイマー運行
	if (animationTimer_.GetIsZero()) {
		panelState_ = END;
		animationTimer_.Reset0();
	} else {
		animationTimer_.ToZero();
	}

	float t = animationTimer_.linearity();

	float scale = 0.8 + 0.2f * t;

	mainPosition.transform.scale.x = scale;
	mainPosition.transform.scale.y = scale;

	for (auto& ptr : sprite_) {
		ptr->objectParts_[0].materialConfig->textureColor.w = t;
	}
}


#ifdef _DEBUG
void Panel::ImGuiPart() {

	ImGui::Begin("Panel");
	ImGui::Text("P/M : %f,%f", animationTimer_.parameter_, animationTimer_.maxTime_);
	ImGui::SliderFloat("cornerPosLT", &animationTimer_.parameter_, 0, animationTimer_.maxTime_);
	ImGui::End();
}
#endif 