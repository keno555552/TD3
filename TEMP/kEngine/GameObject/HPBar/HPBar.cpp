#include "HPBar.h"

HPBar::HPBar(kEngine* system, float maxHP, float nowHP, Object* targetObject, const Vector3 turningPos, const Vector4 color) {
	IntObject(system);
	maxHP_ = maxHP;
	nowHP_ = nowHP;

	barColor_ = color;

	HPBarModelHandle_ = system_->SetModelObj("resources/object/HPBar/HPBarModel.obj");

	barTurningPos_ = turningPos;

	Object::CreateModelData(HPBarModelHandle_);
	mainPosition.transform.translate += barTurningPos_;
	objectParts_[0].materialConfig->textureColor = barColor_;
	isBillboard_ = true;

	LinkToObject(targetObject);
}

void HPBar::Update(Camera* camera) {
	UpdateBarPart();
	Object::Update(camera);
}

void HPBar::Draw() {
	Object::Draw();
}

void HPBar::SetMaxHP(float maxHP) {
	maxHP_ = std::max<float>(0.01f, maxHP);
}

void HPBar::SetNowHP(float nowHP) {
	nowHP_ = std::clamp(nowHP, 0.0f, maxHP_);
}

void HPBar::SetBarColor(const Vector4& color) {
	barColor_ = color;
	if (objectParts_[0].materialConfig->textureColor != barColor_) {
		objectParts_[0].materialConfig->textureColor = barColor_;
	}
}

void HPBar::SetDamageColor(const Vector4& color) {
	DamageColor_ = color;
}

void HPBar::SetBarWidth(float width) {
	if(barWidth_ != width){
		barWidth_ = width;
		barWidthChange_ = true;
	}
}

void HPBar::LinkToObject(Object* targetObject) {
	if(targetObject != nullptr){
		followObject_ = &targetObject->mainPosition;
	}
}

void HPBar::UpdateBarPart() {
	if (barWidthChange_) {
		objectParts_[1].transform.translate.x = -(barWidth_ - 1.0f)/2;
		objectParts_[2].transform.translate.x = +(barWidth_ - 1.0f)/2;
		objectParts_[3].transform.scale.x = barWidth_;
		barWidthChange_ = false;
	}

	float t = nowHP_ / maxHP_;
	objectParts_[0].transform.scale.x = linearity(0.0f, objectParts_[3].transform.scale.x,t);
	objectParts_[0].transform.translate.x = linearity(-barWidth_/2.0f, 0.0f, t);
}
