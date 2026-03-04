#include "button.h"

Button::Button(kEngine* system) {
	system_ = system;

	pos_ = {};
	width_ = {};
	height_ = {};

	textureN_ = {};
	textureS_ = {};
	textureP_ = {};
	textureC_ = {};

	click1_ = {};
	click2_ = {};

	on_ = false;
	isPress_ = false;
	isSelect_ = false;
	canPress_ = true;
}

bool Button::GetPress()
{
	if (isPress_) { return true; }
	return false;
}

bool Button::GetCanPress()
{
	if (canPress_) { return true; }
	return false;
}

bool Button::GetOnOff()
{
	if (on_) {
		return true;
	}
	return false;
}

bool Button::GetResetOnOff()
{
	if (on_) {
		on_ = false;
		return true;
	}
	return false;
}

void Button::SetPos(Vector2 pos)
{
	pos_ = pos;
}

void Button::SetWidth(float width)
{
	width_ = width;
}

void Button::SetHeight(float height)
{
	height_ = height;
}

void Button::SetCanPress()
{
	canPress_ = true;
}

void Button::SetCantPress()
{
	canPress_ = false;
}

bool Button::CheckIsSelect(Vector2 TPos, float width, float height)
{
	if (canPress_) {
		Vector2 A[2] = { { pos_.x - width_ / 2, pos_.y + height_ / 2},{ pos_.x + width_ / 2, pos_.y - height_ / 2 } };
		Vector2 B[2] = { { TPos.x - width / 2, TPos.y + height / 2},{ TPos.x + width / 2, TPos.y - height / 2 } };
		int DFlat[2] = {};

		if (B[0].x < A[1].x) { DFlat[0]++; }
		if (B[1].x > A[0].x) { DFlat[0]++; }

		if (B[0].y > A[1].y) { DFlat[1]++; }
		if (B[1].y < A[0].y) { DFlat[1]++; }

		if (DFlat[0] == 2 && DFlat[1] == 2) {
			isSelect_ = true;
			return true;
		}
		else {
			isSelect_ = false;
			return false;
		}
	}
	return false;
}

void Button::ResetButton()
{
	on_ = false;
}


void Button::Init(Vector2 pos, float width, float height, int textureN, int textureS, int textureP, int textureC, int click1, int click2)
{
	pos_ = pos;
	width_ = width;
	height_ = height;

	textureN_ = textureN;
	textureS_ = textureS;
	textureP_ = textureP;
	textureC_ = textureC;

	click1_ = click1;
	click2_ = click2;
}

void Button::Update()
{
	int mouseX = 0, mouseY = 0;
	mouseX = system_->GetMousePosX();
	mouseY = system_->GetMousePosY();
	if (canPress_) {
		if (CheckIsSelect({ (float)mouseX,(float)mouseY }, 1, 1)) {
			if (system_->GetMouseTriggerOn(0)) {
					if (isPress_ == false) {
						//Novice::PlayAudio(click1_, 0, 0.8f);
					}
				isPress_ = true;
			}
			else {
				isPress_ = false;
			}
			if (isPress_) {
				if (!system_->GetMouseTriggerOn(0)) {
					if (click2_ != 0) {
						if (on_ == false) {
							//Novice::PlayAudio(click2_, 0, 0.8f);
						}
					}
				}
				on_ = true;
			}
		}
		else {
			isPress_ = false;
		}
	}
}

void Button::Render()
{
	if (canPress_) {
		if (!isPress_) {
			if (isSelect_) {
				///選ばれてる
				material_.textureHandle = textureS_;
				//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureS_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
			}
			else {
				///選ばれてない
				material_.textureHandle = textureN_;
				//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureN_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
			}
		}
		else {
			///押されてる
			material_.textureHandle = textureP_;
			//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureP_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
		}
	}
	else {
		///押せない
		material_.textureHandle = textureC_;
		//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureC_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
	}

	//system_->DrawSprite({ (pos_.x - width_ / 2.0f), (pos_.y - height_ / 2.0f) }, material_);

	///Test用
	//Novice::ScreenPrintf(0, 0, "%d", isPress_);
}
