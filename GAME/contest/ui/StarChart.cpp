#include "StarChart.h"
#include "kEngine.h"
#include "Object/Sprite.h"
#include "Data/Render/CPUData/CornerData.h"
#include "Vector4.h"
#include <cmath>

static constexpr float kTwoPi  = 6.28318530717959f;
static constexpr float kHalfPi = 1.57079632679490f;

void StarChart::Initialize(kEngine* system, float centerX, float centerY) {
	system_  = system;
	centerX_ = centerX;
	centerY_ = centerY;

	// 任意形状のクワッドを塗りつぶすため単色白テクスチャを使用
	textureHandle_ = system_->LoadTexture("GAME/resources/texture/white100x100.png");

	for (int i = 0; i < 5; ++i) {
		triangles_[i] = new SimpleSprite;
		triangles_[i]->IntObject(system_);
		triangles_[i]->CreateDefaultData();

		// mainPosition はデフォルト（scale=1, rotate=0, translate=0）のまま使う
		// anchorPoint はデフォルトの {0,0} のまま → conerData が SetSize(CornerData) に渡される

		triangles_[i]->objectParts_[0].materialConfig->textureHandle   = textureHandle_;
		triangles_[i]->objectParts_[0].materialConfig->useModelTexture = false;
	}

	// 初期状態を全 ★0 にして conerData をデフォルト値から外す
	SetStars(0, 0, 0, 0, 0);
}

void StarChart::SetStars(int theme, int impact, int commitment,
	int efficiency, int judgeEval) {

	int stars[5] = { theme, impact, commitment, efficiency, judgeEval };

	// 5つの外側頂点（apex）と5つの内側谷点（dip）を計算
	Vector2 apex[5];
	Vector2 dip[5];

	// Step 1: 各apexの半径と位置を確定
	// ★0 は腕を出さない（半径0で凧形が縮退）
	// ★1〜5 は kMinStarRatio〜1.0 を線形補間
	float apexR[5];
	for (int i = 0; i < 5; ++i) {
		if (stars[i] > 0) {
			float t = (stars[i] - 1) / 4.0f;   // ★1→0, ★5→1
			apexR[i] = kMaxRadius * (kMinStarRatio + t * (1.0f - kMinStarRatio));
		} else {
			apexR[i] = 0.0f;
		}

		float apexAngle = (kTwoPi / 5.0f) * i - kHalfPi;
		apex[i] = {
			centerX_ + apexR[i] * cosf(apexAngle),
			centerY_ + apexR[i] * sinf(apexAngle)
		};
	}

	// Step 2: 各dipの半径と位置を確定
	// 隣接2つのapex半径の平均 × kDipRatio で「少し内側」に。
	// kDipFloorRatio * kMaxRadius を下限にして、apexが小さくても谷が潰れないようにする
	const float dipFloor = kMaxRadius * kDipFloorRatio;
	for (int i = 0; i < 5; ++i) {
		float avgApexR = (apexR[i] + apexR[(i + 1) % 5]) * 0.5f;
		float dipR = avgApexR * kDipRatio;
		if (dipR < dipFloor) dipR = dipFloor;

		float dipAngle = (kTwoPi / 5.0f) * i - kHalfPi + kTwoPi / 10.0f;  // apexAngle + 36°
		dip[i] = {
			centerX_ + dipR * cosf(dipAngle),
			centerY_ + dipR * sinf(dipAngle)
		};
	}

	Vector2 center = { centerX_, centerY_ };

	// 各腕を凧形クワッド (center, leftDip, apex[i], rightDip) として描画
	// = 内側ペンタゴン1スライス + 外側アーム三角形
	//
	// SimpleSpriteMesh の頂点格納はクロス（TL↔BL, TR↔BR）：
	//   mesh[TL].pos = coner[BL]   mesh[BL].pos = coner[TL]
	//   mesh[BR].pos = coner[TR]   mesh[TR].pos = coner[BR]
	// インデックス [TL,BL,BR] [TL,BR,TR] → 実頂点は
	//   Tri1: (coner[BL], coner[TL], coner[TR])
	//   Tri2: (coner[BL], coner[TR], coner[BR])
	//
	// 凧形にするため対角線を center↔apex に取り、
	//   coner[BL]=center, coner[TR]=apex, coner[TL]=leftDip, coner[BR]=rightDip
	// → Tri1: (center, leftDip, apex)
	//   Tri2: (center, apex, rightDip)
	for (int i = 0; i < 5; ++i) {
		Vector2 leftDip  = dip[(i + 4) % 5];   // dip[i-1]：apex[i] の左隣の谷
		Vector2 rightDip = dip[i];             // apex[i] の右隣の谷

		auto& c = triangles_[i]->objectParts_[0].conerData.coner;
		c[(int)CornerName::BOTTOM_LEFT]  = center;
		c[(int)CornerName::TOP_LEFT]     = leftDip;
		c[(int)CornerName::TOP_RIGHT]    = apex[i];
		c[(int)CornerName::BOTTOM_RIGHT] = rightDip;
	}
}

void StarChart::SetColor(const Vector4& color) {
	for (int i = 0; i < 5; ++i) {
		if (triangles_[i]) {
			triangles_[i]->objectParts_[0].materialConfig->textureColor = color;
		}
	}
}

void StarChart::EnableLevelDots(const Vector4& color) {
	const float halfSize = kDotSize * 0.5f;
	int dotIdx = 0;

	for (int axis = 0; axis < 5; ++axis) {
		float angle = (kTwoPi / 5.0f) * axis - kHalfPi;

		for (int level = 1; level <= 5; ++level) {
			// ★level の apex 半径と同じ式（kMinStarRatio〜1.0 の線形補間）
			float t = (level - 1) / 4.0f;
			float r = kMaxRadius * (kMinStarRatio + t * (1.0f - kMinStarRatio));

			float x = centerX_ + r * cosf(angle);
			float y = centerY_ + r * sinf(angle);

			// 既存があれば差し替え
			if (dots_[dotIdx]) {
				delete dots_[dotIdx];
			}
			dots_[dotIdx] = new SimpleSprite;
			dots_[dotIdx]->IntObject(system_);
			dots_[dotIdx]->CreateDefaultData();
			dots_[dotIdx]->objectParts_[0].materialConfig->textureHandle   = textureHandle_;
			dots_[dotIdx]->objectParts_[0].materialConfig->useModelTexture = false;
			dots_[dotIdx]->objectParts_[0].materialConfig->textureColor    = color;

			// (x, y) を中心に kDotSize × kDotSize の四角形
			auto& c = dots_[dotIdx]->objectParts_[0].conerData.coner;
			c[(int)CornerName::TOP_LEFT]     = { x - halfSize, y - halfSize };
			c[(int)CornerName::BOTTOM_LEFT]  = { x - halfSize, y + halfSize };
			c[(int)CornerName::BOTTOM_RIGHT] = { x + halfSize, y + halfSize };
			c[(int)CornerName::TOP_RIGHT]    = { x + halfSize, y - halfSize };

			dotIdx++;
		}
	}
}

void StarChart::Draw() {
	for (int i = 0; i < 5; ++i) {
		if (triangles_[i]) {
			triangles_[i]->Draw();
		}
	}
	// ドットは星形より後に描画して手前に出す
	for (int i = 0; i < 25; ++i) {
		if (dots_[i]) {
			dots_[i]->Draw();
		}
	}
}

void StarChart::Cleanup() {
	for (int i = 0; i < 5; ++i) {
		delete triangles_[i];
		triangles_[i] = nullptr;
	}
	for (int i = 0; i < 25; ++i) {
		delete dots_[i];
		dots_[i] = nullptr;
	}
}
