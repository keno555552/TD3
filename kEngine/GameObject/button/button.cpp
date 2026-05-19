#include "button.h"

Button::Button(kEngine* system) {
	system_ = system;

	IntObject(system_);
	CreateDefaultData();

	click1_ = {};
	click2_ = {};

	on_ = false;
	isPress_ = false;
	isSelect_ = false;
	canPress_ = true;
}

bool Button::GetResetOnOff() {
	if (on_) {
		on_ = false;
		return true;
	}
	return false;
}

void Button::SetPos(Vector2 pos) {

	mainPosition.transform.translate.x = pos.x;
	mainPosition.transform.translate.y = pos.y;

}

void Button::SetWidth(float width) {
	SetCornerWidth(width);
	width_ = width;
}

void Button::SetHeight(float height) {
	SetCornerHeight(height);
	height_ = height;
}

bool Button::GetIsSelect(Vector2 TPos, float width, float height) {

	Vector2 followPos = GetFollowPosition();

	if (canPress_) {
		Vector2 A[2] = { { followPos.x - width_ / 2, followPos.y + height_ / 2},{ followPos.x + width_ / 2, followPos.y - height_ / 2 } };
		Vector2 B[2] = { { TPos.x - width / 2, TPos.y + height / 2},{ TPos.x + width / 2, TPos.y - height / 2 } };
		int DFlat[2] = {};

		if (B[0].x < A[1].x) { DFlat[0]++; }
		if (B[1].x > A[0].x) { DFlat[0]++; }

		if (B[0].y > A[1].y) { DFlat[1]++; }
		if (B[1].y < A[0].y) { DFlat[1]++; }

		if (DFlat[0] == 2 && DFlat[1] == 2) {
			isSelect_ = true;
			return true;
		} else {
			isSelect_ = false;
			return false;
		}
	}
	return false;
}

void Button::ClearAllTexture() {
	textureN_ = -1;
	textureS_ = -1;
	textureP_ = -1;
	textureC_ = -1;
}


void Button::SetFollowObject(SpriteData* followObject) {
	followObject_ = followObject;
}

void Button::ResetButton() {
	on_ = false;
}


void Button::Init(Vector3 pos, float width, float height, int textureN, int textureS, int textureP, int textureC, int click1, int click2) {
	mainPosition.transform.translate.x = pos.x;
	mainPosition.transform.translate.y = pos.y;
	mainPosition.transform.translate.z = pos.z;
	SetCorner(width, height);
	width_ = width;
	height_ = height;

	textureN_ = textureN;
	textureS_ = textureS;
	textureP_ = textureP;
	textureC_ = textureC;

	click1_ = click1;
	click2_ = click2;
}

void Button::Update() {
	int mouseX = 0, mouseY = 0;
	mouseX = system_->GetMousePosX();
	mouseY = system_->GetMousePosY();

	bool hover = GetIsSelect({ (float)mouseX, (float)mouseY }, 1, 1);
	bool press = system_->GetMouseIsPush(0);
	bool triggerOn = system_->GetMouseTriggerOn(0);
	bool triggerOff = system_->GetMouseTriggerOff(0);

	// 點擊事件（只維持一幀）
	isClick_ = (triggerOn && hover);

	// 記錄是否曾經按下
	if (triggerOn && hover) {
		isPress_ = true;
		wasPressed_ = true;
		isClick_ = true;
	}

	// 放開事件（只在曾經按下後才觸發）
	if (isRelease_) {
		isRelease_ = false;
	}
	if (triggerOff) {
		if(wasPressed_) {
			isRelease_ = true;
		}
		isPress_ = false;
		wasPressed_ = false;
	}

	// 按住狀態（不依賴 hover）
	if (press && wasPressed_) {
		isPress_ = true;
	}
}

void Button::Render() {
	auto& material = objectParts_[0].materialConfig;
	if (canPress_) {
		if (!isPress_) {
			if (isSelect_) {
				///選ばれてる
				if (textureS_ != -1) {
					material->textureHandle = textureS_;
					material->textureColor = textureColorS_;
					Draw();
				}
			} else {
				if (textureN_ != -1) {
					///選ばれてない
					material->textureHandle = textureN_;
					material->textureColor = textureColorN_;
					Draw();
					//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureN_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
				}
			}
		} else {
			///押されてる
			if (textureP_ != -1) {
				material->textureHandle = textureP_;
				material->textureColor = textureColorP_;
				Draw();
				//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureP_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
			}
		}
	} else {
		///押せない
		if (textureC_ != -1) {
			material->textureHandle = textureC_;
			material->textureColor = textureColorC_;
			Draw();
			//Novice::DrawSprite(int(pos_.x - width_ / 2), int(pos_.y - height_ / 2), textureC_, 1.0f, 1.0f, 0.0f, 0xFFFFFFFF);
		}
	}

	//system_->DrawSprite({ (pos_.x - width_ / 2.0f), (pos_.y - height_ / 2.0f) }, material_);

	///Test用
	//Novice::ScreenPrintf(0, 0, "%d", isPress_);
}


void Button::SetCorner(float width, float height) {
	SetCornerWidth(width);
	SetCornerHeight(height);
}

void Button::SetCornerWidth(float width) {
	float halfWidth = width / 2.0f;
	objectParts_[0].conerData.coner[0].x = -halfWidth;
	objectParts_[0].conerData.coner[1].x = -halfWidth;
	objectParts_[0].conerData.coner[2].x = halfWidth;
	objectParts_[0].conerData.coner[3].x = halfWidth;

}

void Button::SetCornerHeight(float height) {
	float halfHeight = height / 2.0f;
	objectParts_[0].conerData.coner[0].y = -halfHeight;
	objectParts_[0].conerData.coner[1].y = halfHeight;
	objectParts_[0].conerData.coner[2].y = halfHeight;
	objectParts_[0].conerData.coner[3].y = -halfHeight;

}

Vector2 Button::GetFollowPosition() {

	float zBuffer = mainPosition.transform.translate.z;

	Matrix4x4 objectMainMatrix = MakeAffineMatrix(
		mainPosition.transform.scale,
		mainPosition.transform.rotate,
		mainPosition.transform.translate
	);

	Matrix4x4 parentMatrix = Identity();

	SpriteData* parent = followObject_;
	while (parent != nullptr) {
		Matrix4x4 local = MakeAffineMatrix(
			parent->mainPosition.transform.scale,
			parent->mainPosition.transform.rotate,
			parent->mainPosition.transform.translate
		);

		parentMatrix = local * parentMatrix;
		parent = parent->followObject_;
	}

	Matrix4x4 resultMatrix = objectMainMatrix * parentMatrix;

	return Vector2{resultMatrix.m[3][0], resultMatrix.m[3][1]};
}
