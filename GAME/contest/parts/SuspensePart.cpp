#include "SuspensePart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"
#include <cmath>
#include <random>

SuspensePart::SuspensePart(kEngine* system, BitmapFont* font,
	Light* lights[3], const Vector3 targetPositions[3],
	const std::vector<int>& winnerIndices)
	: IContestPart(system, font), winnerIndices_(winnerIndices) {

	cameraTransform_ = { { 0.0f, 0.9f, -3.0f }, { 0.12f, 0.0f, 0.0f } };

	for (int i = 0; i < 3; ++i) {
		lights_[i] = lights[i];
		targetPositions_[i] = targetPositions[i];
		if (lights_[i]) {
			lightOriginalDir_[i] = lights_[i]->direction;
			lightOriginalColor_[i] = lights_[i]->color;
			originalIntensity_[i] = lights_[i]->intensity;
			revealStartDir_[i] = lights_[i]->direction;
		}
	}
	for (int w : winnerIndices_) {
		if (w >= 0 && w < 3) isWinner_[w] = true;
	}

	// 演出パターンをランダム抽選し、Feint用の非当選キャラを決定
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> distPattern(0, 2);
		revealPattern_ = static_cast<RevealPattern>(distPattern(gen));

		// 非当選インデックスを集めて1つ選ぶ
		std::vector<int> nonWinners;
		for (int i = 0; i < 3; ++i) {
			if (!isWinner_[i]) nonWinners.push_back(i);
		}
		if (!nonWinners.empty()) {
			std::uniform_int_distribution<int> distDecoy(0, static_cast<int>(nonWinners.size()) - 1);
			decoyIdx_ = nonWinners[distDecoy(gen)];
		} else {
			// 全員当選など非当選者がいない場合はFeint不可 → Slowに劣化
			decoyIdx_ = -1;
			if (revealPattern_ == RevealPattern::Feint) {
				revealPattern_ = RevealPattern::Slow;
			}
		}
	}

	drawButton_ = std::make_unique<DetailButton>(system);
	drawButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);

	// サウンドロード
	drumrollHandle_ = system_->SoundLoadSE("GAME/resources/sounds/drumroll.wav");
	heartbeatHandle_ = system_->SoundLoadSE("GAME/resources/sounds/心臓の鼓動1.mp3");
	spotlightSeHandle_ = system_->SoundLoadSE("GAME/resources/sounds/SpotLight.mp3");
	cheersHandle_ = system_->SoundLoadSE("GAME/resources/sounds/Cheers.mp3");

	// Suspense開始でドラムロール
	if (drumrollHandle_ != -1) {
		system_->SoundPlayBGM(drumrollHandle_, 0.8f);
	}
}

SuspensePart::~SuspensePart() {
	// 決定後のライト状態は次パートに引き継ぐので復元しない
	// 鳴りっぱなしを防ぐためループ系SEは停止
	if (drumrollHandle_ != -1) system_->SoundStop(drumrollHandle_);
	if (heartbeatHandle_ != -1) system_->SoundStop(heartbeatHandle_);
}

void SuspensePart::Update() {
	timer_ += 1.0f / 60.0f;

	if (state_ == State::Suspense) {
		drawButton_->Update();

		// 各ライトの向きをXZ軸で揺らす（位置は不変）
		// 各ライトに位相オフセットを与え個別に揺れさせる
		for (int i = 0; i < 3; ++i) {
			if (!lights_[i]) continue;
			lights_[i]->direction = SwayDir(i);

			// 色を HSV で巡回（ライトごとに位相をずらす）
			float hue = std::fmod(timer_ * 0.35f + static_cast<float>(i) * 0.33f, 1.0f);
			Vector3 rgb = HsvToRgb(hue, 0.85f, 1.0f);
			lights_[i]->color = rgb;
		}

		if (system_->GetTriggerOn(DIK_SPACE) || drawButton_->GetIsRelease()) {
			// 当選者へ向ける開始
			state_ = State::Revealing;
			revealTimer_ = 0.0f;
			for (int i = 0; i < 3; ++i) {
				if (lights_[i]) {
					revealStartDir_[i] = lights_[i]->direction;
				}
			}
			// ドラムロール停止 → 心臓の鼓動ループ開始
			if (drumrollHandle_ != -1) system_->SoundStop(drumrollHandle_);
			if (heartbeatHandle_ != -1) system_->SoundPlayBGM(heartbeatHandle_, 0.9f);
		}
		return;
	}

	if (state_ == State::Revealing) {
		revealTimer_ += 1.0f / 60.0f;
		float t = revealTimer_ / revealDuration_;
		if (t >= 1.0f) t = 1.0f;

		bool soloWinner = (winnerIndices_.size() == 1);
		int soloIdx = soloWinner ? winnerIndices_[0] : -1;

		for (int i = 0; i < 3; ++i) {
			if (!lights_[i]) continue;

			// このライトが向けるべきターゲットインデックスを決定
			// 単独1位: 全ライトがその人へ
			// 同率1位: 当選者本人のライトは自分自身へ、それ以外は消灯
			int aimIdx = -1;
			bool keepLit = true;
			if (soloWinner) {
				aimIdx = soloIdx;
			} else {
				if (isWinner_[i]) {
					aimIdx = i;
				} else {
					keepLit = false;
				}
			}

			if (aimIdx >= 0) {
				Vector3 lp = lights_[i]->position;
				Vector3 toWinner = AimDir(lp, targetPositions_[aimIdx]);
				Vector3 start = revealStartDir_[i];

				Vector3 cur;
				switch (revealPattern_) {
				case RevealPattern::Slow:
				default:
					// 全区間ゆっくり当選者へ（従来）
					cur = LerpDir(start, toWinner, t);
					break;

				case RevealPattern::Snap:
					// snapStart_ までは揺らし続けて待ち、以降で一気に当選者へ
					if (t < snapStart_) {
						cur = SwayDir(i);
					} else {
						float st = (t - snapStart_) / (1.0f - snapStart_);
						cur = LerpDir(SwayDir(i), toWinner, st);
					}
					break;

				case RevealPattern::Feint: {
					// 一旦非当選者へ向かい、溜めてからパッと当選者へ
					Vector3 toDecoy = (decoyIdx_ >= 0)
						? AimDir(lp, targetPositions_[decoyIdx_])
						: toWinner;
					if (t < feintAimEnd_) {
						cur = LerpDir(start, toDecoy, t / feintAimEnd_);
					} else if (t < feintHoldEnd_) {
						cur = toDecoy;
					} else {
						float ft = (t - feintHoldEnd_) / (1.0f - feintHoldEnd_);
						cur = LerpDir(toDecoy, toWinner, ft);
					}
					break;
				}
				}

				lights_[i]->direction = NormalizeSafe(cur);
			}

			// 色は白へ収束
			lights_[i]->color = {
				lights_[i]->color.x + (1.0f - lights_[i]->color.x) * t,
				lights_[i]->color.y + (1.0f - lights_[i]->color.y) * t,
				lights_[i]->color.z + (1.0f - lights_[i]->color.z) * t,
			};

			// 非当選ライトは intensity を 0 へフェード（消灯）
			if (!keepLit) {
				lights_[i]->intensity = originalIntensity_[i] * (1.0f - t);
			} else {
				lights_[i]->intensity = originalIntensity_[i];
			}
		}

		if (t >= 1.0f) {
			state_ = State::Done;
			afterRevealTimer_ = 0.0f;
			// 心臓の鼓動停止 → スポットライトSE+歓声SE
			if (heartbeatHandle_ != -1) system_->SoundStop(heartbeatHandle_);
			if (spotlightSeHandle_ != -1) system_->SoundPlaySE(spotlightSeHandle_, 1.0f);
			if (cheersHandle_ != -1) system_->SoundPlaySE(cheersHandle_, 1.0f);
		}
		return;
	}

	// Done: 余韻
	afterRevealTimer_ += 1.0f / 60.0f;
}

void SuspensePart::Draw() {
	if (state_ == State::Suspense) {
		font_->RenderText("一位は誰だ！?", { 640.0f, 100.0f }, 56.0f,
			BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.4f, 1.0f });
		drawButton_->Render();
		font_->RenderText("Stop", { 640.0f, 630.0f }, 48.0f,
			BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.0f, 1.0f });
	} else if (state_ == State::Revealing) {
		font_->RenderText("...", { 640.0f, 100.0f }, 64.0f,
			BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 1.0f, 1.0f });
	} else {
		font_->RenderText("一位はこいつだ!", { 640.0f, 100.0f }, 72.0f,
			BitmapFont::Align::Center, 5, { 1.0f, 0.9f, 0.2f, 1.0f });
	}
}

bool SuspensePart::IsFinished() const {
	return state_ == State::Done && afterRevealTimer_ >= afterRevealHold_;
}

PartCameraTransform SuspensePart::GetCameraTransform() const {
	return cameraTransform_;
}

Vector3 SuspensePart::NormalizeSafe(const Vector3& v) {
	float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len < 1e-6f) return { 0.0f, -1.0f, 0.0f };
	return { v.x / len, v.y / len, v.z / len };
}

Vector3 SuspensePart::SwayDir(int i) const {
	float phase = static_cast<float>(i) * 1.7f;
	float sx = std::sin(timer_ * swayFreq_ + phase) * swayAmp_;
	float sz = std::cos(timer_ * swayFreq_ * 0.9f + phase * 1.3f) * swayAmp_;
	return NormalizeSafe({ sx, -1.0f, sz });
}

Vector3 SuspensePart::AimDir(const Vector3& pos, const Vector3& target) {
	return NormalizeSafe({ target.x - pos.x, target.y - pos.y, target.z - pos.z });
}

Vector3 SuspensePart::LerpDir(const Vector3& a, const Vector3& b, float t) {
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t,
	};
}

Vector3 SuspensePart::HsvToRgb(float h, float s, float v) {
	float r = 0.0f, g = 0.0f, b = 0.0f;
	int i = static_cast<int>(std::floor(h * 6.0f));
	float f = h * 6.0f - static_cast<float>(i);
	float p = v * (1.0f - s);
	float q = v * (1.0f - f * s);
	float t = v * (1.0f - (1.0f - f) * s);
	switch (i % 6) {
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}
	return { r, g, b };
}
