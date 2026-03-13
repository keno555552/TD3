#include "DebugObject.h"

DebugObject::DebugObject(kEngine* kEngine) {
	system_ = kEngine;
	TH_centerPoint = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/centerPoint.png");
	TH_number = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/number/unicode_japaness_number.png");
}

DebugObject::~DebugObject() {
	centerPoint_.reset();
}

void DebugObject::Update(Camera* camera) {
	if (camera) {
		camera_ = camera;
		updateDebugParts();
	}
}

void DebugObject::Draw() {
	if (centerPoint_) centerPoint_->Draw();
	if (centerNumberSprite_) centerNumberSprite_->Draw();
}

void DebugObject::SetFollowObject(ObjectPart* followObject) {
	followObject_ = followObject;
}

void DebugObject::ClearFollowObject() {
	followObject_ = nullptr;
}

void DebugObject::SetShowCenterPoint(bool isShow) {
	isShowCenterPoint_ = isShow;
	if (isShowCenterPoint_) {
		if (centerPoint_ == nullptr) {
			centerPoint_ = std::make_unique<SimpleSprite>();
			centerPoint_->IntObject(system_);
			centerPoint_->CreateDefaultData();
			centerPoint_->objectParts_[0].anchorPoint = { 15.0f,15.0f };
			centerPoint_->objectParts_[0].materialConfig->textureHandle = TH_centerPoint;
		}
	} else {
		if (centerPoint_) {
			centerPoint_.reset();
		}
	}
}

void DebugObject::SetShowNumber(bool isShow) {
	isCenterNumber_ = isShow;
	if (isCenterNumber_) {
		if (centerNumberSprite_ == nullptr) {
			centerNumberSprite_ = std::make_unique<SimpleSprite>();
			centerNumberSprite_->IntObject(system_);
			centerNumberSprite_->mainPosition.materialConfig = std::make_shared<MaterialConfig>();
			centerNumberSprite_->mainPosition.materialConfig->lightModelType = LightModelType::Sprite2D;
			centerNumberSprite_->mainPosition.materialConfig->textureHandle = TH_number;
			centerNumberSprite_->mainPosition.materialConfig->textureColor = { 1,1,1,1 };
		}
	} else {
		if (centerNumberSprite_) {
			centerNumberSprite_.reset();
		}
	}
}

void DebugObject::UpdateShowNumber(int Number) {
	centerNumber_ = Number;
}


/// ============= 全体更新 ============= ///
void DebugObject::updateDebugParts() {
	if (!isNeedUpdate())return;
	updateTargetPosition();
	updateCenterPoint();
	updateCenterNumber();
}

/// ターゲットポジション更新 ///
void DebugObject::updateTargetPosition() {
	targetPosition_ = followObject_
		? followObject_->transform.translate
		: mainPosition.transform.translate;
}
/// 中心点更新 ///
void DebugObject::updateCenterPoint() {
	if (isShowCenterPoint_) {
		if (centerPoint_ != nullptr) {
			Vector2 pos = camera_->GetObjectScreenPos(followObject_->transform.translate);
			centerPoint_->mainPosition.transform.translate = Vector3(pos.x, pos.y, 0.0f);
		}
	}
}

void DebugObject::updateCenterNumber() {
	if (isCenterNumber_) {
		if (centerNumberSprite_ != nullptr) {

			/// 数字分解 ///
			centerNumberArray.clear();
			int number = centerNumber_;
			if (number == 0) {
				centerNumberArray.push_back(0);
			} else {
				while (number > 0) {
					int digit = number % 10;
					centerNumberArray.push_back(digit);
					number /= 10;
				}
			}
			int numDigits = (int)centerNumberArray.size();

			/// スプライト数調整 ///
			int diff = (int)centerNumberSprite_->objectParts_.size() - numDigits;
			if (diff < 0) {
				for (int i = 0; i < -diff; i++) {
					centerNumberSprite_->CreateDefaultData();
					centerNumberSprite_->objectParts_.back().materialConfig = centerNumberSprite_->mainPosition.materialConfig;
				}
			} else if (diff > 0) {
				for (int i = 0; i < diff; i++) {
					centerNumberSprite_->objectParts_.pop_back();
				}
			}

			/// スプライト配置 ///

			float scale = 0.025f;

			float spriteW = 1140.0f;
			float spriteH = 160.0f;

			float cropW = 114.0f;
			float cropH = 160.0f;

			float spacing = 2.0f;

			// スケール適用後のサイズ
			float pxScale = scale * 10; // ピクセルをスクリーン座標に変換するためのスケール
			float scaledCropW = cropW * pxScale;
			float scaledCropH = cropH * pxScale;
			float scaledSpacing = spacing * pxScale;

			// 整排寬度
			float totalWidth = spriteW * numDigits + spacing * (numDigits - 1);


			// 左上角基準
			Vector2 pos = camera_->GetObjectScreenPos(targetPosition_);
			float baseX = - totalWidth * 0.5f;
			float baseY = - spriteH * 0.5f;

			for (int i = 0; i < numDigits; i++) {

				int digit = centerNumberArray[numDigits - 1 - i];
				auto& part = centerNumberSprite_->objectParts_[i];

				// 設定裁切
				part.cropLT = Vector2(digit * cropW, 0.0f);
				part.cropSize = Vector2(cropW, cropH);

				// 設定位置（中心點）
				float x = baseX + i * (spriteW + spacing);
				part.transform.translate = { x,baseY,0 };
			}


			// 設定縮放
			float scaleX = scale;
			float scaleY = scale * 10.0f;
			centerNumberSprite_->mainPosition.transform.scale = Vector3(scaleX, scaleY, 1.0f);
			centerNumberSprite_->mainPosition.transform.translate = Vector3(pos.x, pos.y, 0.0f);
		}
	}
}

bool DebugObject::isNeedUpdate() {
	if (isShowCenterPoint_) return true;
	if (isCenterNumber_) return true;
	return false;
}


