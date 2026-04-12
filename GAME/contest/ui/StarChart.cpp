#include "StarChart.h"
#include "kEngine.h"
#include "Object/Sprite.h"

void StarChart::Initialize(kEngine* system, float centerX, float centerY) {
	system_ = system;
	centerX_ = centerX;
	centerY_ = centerY;

	// 三角形テクスチャ読み込み
	textureHandle_ = system_->LoadTexture("GAME/resources/texture/triangle.png");

	// 5枚の三角形スプライトを生成
	for (int i = 0; i < 5; ++i) {
		triangles_[i] = new SimpleSprite;
		triangles_[i]->IntObject(system_);
		triangles_[i]->CreateDefaultData();

		// 位置設定（全て同じ中心座標）
		triangles_[i]->mainPosition.transform = CreateDefaultTransform();
		triangles_[i]->mainPosition.transform.translate = {
			centerX_, centerY_, 10.0f
		};

		// アンカーポイント: 底辺の中央（画像座標で128, 256）
		triangles_[i]->objectParts_[0].anchorPoint = { 128.0f, 256.0f };

		// テクスチャ設定
		triangles_[i]->objectParts_[0].materialConfig->textureHandle = textureHandle_;
		triangles_[i]->objectParts_[0].materialConfig->useModelTexture = false;

		// 回転設定（72度ずつ）
		triangles_[i]->mainPosition.transform.rotate = { 0.0f, 0.0f, kRotations[i] };

		// 初期スケール（非表示）
		triangles_[i]->mainPosition.transform.scale = { 0.0f, 0.0f, 1.0f };
	}
}

void StarChart::SetStars(int theme, int impact, int commitment,
	int efficiency, int judgeEval) {

	int stars[5] = { theme, impact, commitment, efficiency, judgeEval };

	for (int i = 0; i < 5; ++i) {
		float scale = StarToScale(stars[i]);
		triangles_[i]->mainPosition.transform.scale = { scale, scale, 1.0f };
	}
}

void StarChart::Draw() {
	for (int i = 0; i < 5; ++i) {
		if (triangles_[i]) {
			triangles_[i]->Draw();
		}
	}
}

void StarChart::Cleanup() {
	for (int i = 0; i < 5; ++i) {
		delete triangles_[i];
		triangles_[i] = nullptr;
	}
}

float StarChart::StarToScale(int star) {
	if (star <= 0) return 0.0f;
	if (star > 5) star = 5;
	// ★1=0.2, ★2=0.4, ★3=0.6, ★4=0.8, ★5=1.0
	return static_cast<float>(star) * 0.2f;
}
