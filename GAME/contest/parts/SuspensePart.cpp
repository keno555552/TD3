#include "SuspensePart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"
#include <cmath>
#include <random>

SuspensePart::SuspensePart(kEngine* system, BitmapFont* font,
	Light* lights[3], const Vector3 targetPositions[3],
	const std::vector<int>& winnerIndices,
	ModCustomizedBodyActor* actors[3],
	Light* envLight,
	Camera* camera)
	: IContestPart(system, font), winnerIndices_(winnerIndices), envLight_(envLight), camera_(camera) {

	cameraTransform_ = { { 0.0f, 0.9f, -3.0f }, { 0.12f, 0.0f, 0.0f } };

	for (int i = 0; i < 3; ++i) {
		lights_[i] = lights[i];
		targetPositions_[i] = targetPositions[i];
		actors_[i] = actors[i];
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

	if (envLight_) {
		envLightOriginalIntensity_ = envLight_->intensity;
		envLight_->intensity = envLightOriginalIntensity_ * 0.1f; // 暗転
	}
	
	confetti_ = std::make_unique<ConfettiParticle>(system_);



	drawButton_ = std::make_unique<DetailButton>(system);
	drawButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);

	// サウンドロード（メモリリーク防止のため静的キャッシュ）
	static int s_drumrollHandle = -1;
	static int s_heartbeatHandle = -1;
	static int s_spotlightSeHandle = -1;
	static int s_cheersHandle = -1;

	if (s_drumrollHandle == -1) {
		s_drumrollHandle = system_->SoundLoadSE("GAME/resources/sounds/drumroll.wav");
		s_heartbeatHandle = system_->SoundLoadSE("GAME/resources/sounds/心臓の鼓動1.mp3");
		s_spotlightSeHandle = system_->SoundLoadSE("GAME/resources/sounds/SpotLight.mp3");
		s_cheersHandle = system_->SoundLoadSE("GAME/resources/sounds/Cheers.mp3");
	}

	drumrollHandle_ = s_drumrollHandle;
	heartbeatHandle_ = s_heartbeatHandle;
	spotlightSeHandle_ = s_spotlightSeHandle;
	cheersHandle_ = s_cheersHandle;

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
	
	if (envLight_) {
		envLight_->intensity = envLightOriginalIntensity_; // 復元
	}
}

void SuspensePart::Update() {
	timer_ += 1.0f / 60.0f;

	if (state_ == State::Suspense) {
		drawButton_->Update();

		// Camera Switching
		cutTimer_ += 1.0f / 60.0f;
		if (cutTimer_ >= nextCutTime_) {
			cutTimer_ = 0.0f;
			currentCutIndex_++;
			if (currentCutIndex_ > 3) currentCutIndex_ = 0;

			if (currentCutIndex_ < 3) {
				Vector3 target = targetPositions_[currentCutIndex_];
				if (actors_[currentCutIndex_]) {
					target = actors_[currentCutIndex_]->GetHeadWorldPosition();
				}
				cameraTransform_.position = { target.x, target.y, target.z - 1.4f };
				cameraTransform_.rotation = { 0.0f, 0.0f, 0.0f };
			} else {
				// 中央からの全体ヒキ絵の時は、適当な平均的高さとして 0.6f 程度を維持
				cameraTransform_.position = { 0.0f, 0.6f, -3.0f };
				cameraTransform_.rotation = { 0.0f, 0.0f, 0.0f };
			}
		}

		// アクターの息づかい（Y軸スケールを揺らす）
		for (int i = 0; i < 3; ++i) {
			if (actors_[i]) {
				float breath = 1.0f + std::sin(timer_ * 5.0f + i) * 0.02f;
				actors_[i]->SetActorScale({ 0.03f, 0.03f * breath, 0.03f });
			}
		}

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
		
		// スペースキーで演出を即スキップ
		if (system_->GetTriggerOn(DIK_SPACE)) {
			revealTimer_ = revealDuration_;
		}

		float t = revealTimer_ / revealDuration_;
		if (t >= 1.0f) t = 1.0f;

		// ゆっくりズームする演出を廃止し、全体を映したまま固定
		cameraTransform_.position = { 0.0f, 0.6f, -3.0f };
		cameraTransform_.rotation = { 0.0f, 0.0f, 0.0f };

		// アクターのスケールを戻す
		for (int i = 0; i < 3; ++i) {
			if (actors_[i]) {
				actors_[i]->SetActorScale({ 0.03f, 0.03f, 0.03f });
			}
		}

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
				Vector3 toWinner = AimDir(lights_[i]->position, targetPositions_[aimIdx]);
				Vector3 cur;

				if (t < snapStart_) {
					cur = SwayDir(i); // 揺れ続ける
				} else {
					// 最後の0.3秒で急激に勝者へ向く
					float st = (t - snapStart_) / (1.0f - snapStart_);
					float easeT = EaseOutExpo(st);
					cur = LerpDir(SwayDir(i), toWinner, easeT);
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

			// 環境光を戻す
			if (envLight_) {
				envLight_->intensity = envLightOriginalIntensity_;
			}
			isFlashing_ = true;
			flashTimer_ = 0.0f;

			// アニメーションを自動再生モードにする
			for (int i = 0; i < 3; ++i) {
				if (actors_[i]) {
					if (isWinner_[i]) {
						actors_[i]->SetJoyAnimationEnabled(true);
					} else {
						actors_[i]->SetFrustrationAnimationEnabled(true);
					}
				}
			}
		}
		return;
	}

	// Done: 余韻
	afterRevealTimer_ += 1.0f / 60.0f;
	
	for (int i = 0; i < 3; ++i) {
		if (actors_[i]) {
			if (isWinner_[i]) {
				// 最初のジャンプ時のみ少し回転させる
				float spin = (afterRevealTimer_ < 0.5f) ? afterRevealTimer_ * 12.0f : 0.0f; // 約1回転
				if (spin > 6.28f) spin = 6.28f;
				actors_[i]->SetActorRotate({ 0.0f, 3.1415f + spin, 0.0f });
			}
		}
	}

	// 発表直後は当選者にカメラを向ける
	bool soloWinner = (winnerIndices_.size() == 1);
	int winIdx = soloWinner ? winnerIndices_[0] : 0;
	Vector3 winTarget = targetPositions_[winIdx];
	if (actors_[winIdx]) {
		winTarget = actors_[winIdx]->GetHeadWorldPosition();
		// カメラがジャンプの上下運動に追従してガタガタ揺れるのを防ぐため、
		// 現在のジャンプ高さ（GroundOffsetY）を差し引いた「本来の頭の高さ」を注視点にする
		winTarget.y -= actors_[winIdx]->GetGroundOffsetY();
	}
	
	// プレイヤー（主人公）の座標
	Vector3 playerTarget = targetPositions_[0];
	if (actors_[0]) {
		playerTarget = actors_[0]->GetHeadWorldPosition();
		playerTarget.y -= actors_[0]->GetGroundOffsetY();
	}

	// 2.0秒以降はプレイヤーへ滑らかにカメラを移動させ、さらに少し寄る（ズーム）
	Vector3 currentTarget = winTarget;
	// さらに引きの画角にするため、-1.8f から -2.5f に変更
	float currentZOffset = -2.5f;

	if (afterRevealTimer_ > 2.0f) {
		float lerpT = (afterRevealTimer_ - 2.0f) / 1.0f; // 1秒かけて移動
		if (lerpT > 1.0f) lerpT = 1.0f;
		
		// イージング（SmoothStep）
		float easeT = lerpT * lerpT * (3.0f - 2.0f * lerpT);

		currentTarget.x = winTarget.x + (playerTarget.x - winTarget.x) * easeT;
		currentTarget.y = winTarget.y + (playerTarget.y - winTarget.y) * easeT;
		currentTarget.z = winTarget.z + (playerTarget.z - winTarget.z) * easeT;
		
		// 移動しながらプレイヤーの顔にしっかり寄るが、審査員（z=-2.0）よりは前に出る（-2.5f から -1.3f へ）
		currentZOffset = -2.5f + ((-1.3f) - (-2.5f)) * easeT;
	}
	
	float shakeX = 0.0f;
	float shakeY = 0.0f;
	if (afterRevealTimer_ < 0.5f) {
		shakeX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.15f;
		shakeY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.15f;
	}
	// 少し見下ろすハイアングルにするため、Yを少し上げ、X回転（下向き）をつける
	cameraTransform_.position = { currentTarget.x + shakeX, currentTarget.y + 0.4f + shakeY, currentTarget.z + currentZOffset };
	cameraTransform_.rotation = { 0.20f, 0.0f, 0.0f };

	// 紙吹雪
	if (!hasSpawnedConfetti_ && afterRevealTimer_ > 0.05f) {
		hasSpawnedConfetti_ = true;
		if (confetti_) {
			confetti_->Spawn({ winTarget.x, winTarget.y + 3.0f, winTarget.z }, 2.0f);
		}
	}

	if (confetti_) {
		confetti_->Update(camera_);
	}
	
	// Flash
	if (isFlashing_) {
		flashTimer_ += 1.0f / 60.0f;
		if (flashTimer_ >= flashDuration_) {
			isFlashing_ = false;
		}
	}
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

	if (confetti_) {
		confetti_->Draw();
	}

	if (isFlashing_) {
		float alpha = 1.0f - (flashTimer_ / flashDuration_);
		if (alpha < 0.0f) alpha = 0.0f;
		
		// 簡易フラッシュ: 白い四角を画面全体に描画
		// BitmapFontの代わりに、Sprite等があれば使うのがよいですが、
		// もし無ければRenderTextで巨大な全角スペース（■）等を描画するか、
		// 既存のフェード処理をここで呼ぶのが理想です。
		// ここではとりあえず巨大な文字で画面を白く覆います（緊急手段）
		font_->RenderText("■■■■■■■■■", { 640.0f, 360.0f }, 800.0f,
			BitmapFont::Align::Center, 10.0f, { 1.0f, 1.0f, 1.0f, alpha });
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

float SuspensePart::EaseOutExpo(float t) {
	return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
