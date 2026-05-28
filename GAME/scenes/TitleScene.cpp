#include "TitleScene.h"
#include "Data/Render/CPUData/ObjectData.h"
#include "kEngine.h"
#include "kEngine/GameObject/Particle/Particle.h"
#include "GAME/effect/DustParticle.h"
#include <memory>

#include "TravelScene.h"
#include "ModScene.h"
#include "GAME/actor/ModCustomizeDataStore.h"
#include <random>
#include <cmath>

#ifdef _DEBUG
#include "ImGuiManager.h"
#endif
TitleScene::TitleScene(kEngine* system) {
	system_ = system;

	TravelScene::ResetTutorialFlag();
	ModScene::ResetTutorialFlag();

	// 最低限のライト
	light1_ = new Light;
	light1_->direction = { -0.5f, -1.0f, -0.3f };
	light1_->color = { 1.0f, 1.0f, 1.0f };
	light1_->intensity = 1.0f;
	system_->AddLight(light1_);

	// 最低限のカメラ
	debugCamera_ = system_->CreateDebugCamera();
	camera_ = system_->CreateCamera();
	titleCamera_ = system_->CreateCamera(); // タイトルロゴ用（デフォルト位置）

	// TravelScene と同じ横視点（NPC が左→右に流れる構図）
	camera_->SetTranslate({ 48.0f, 5.0f, 5.0f });
	camera_->SetDefaultTransform(camera_->GetTransform());
	camera_->SetRotation({ 0.0f, -1.57f, 0.0f });

	usingCamera_ = camera_;
	system_->SetCamera(usingCamera_);

	// フェード
	fade_.Initialize(system_);
	fade_.StartFadeIn();

	// 背景オブジェクト初期化
	BGObjectHandle_ = system_->SetModelObj("GAME/resources/TitleScene/titleBG.obj");
	BGObject_ = std::make_unique<Object>();
	BGObject_->IntObject(system_);
	BGObject_->CreateModelData(BGObjectHandle_);
	BGObject_->mainPosition.transform = CreateDefaultTransform();
	BGObject_->mainPosition.transform.translate.z = 200.0f;
	
	// 平面(Plane)の設定: カリングを回避するためにY軸で180度回転し、色を暗めに
	BGObject_->objectParts_[0].transform.rotate.y = 3.14159265f;
	BGObject_->objectParts_[0].materialConfig->enableLighting = false;
	Vector4 bgColorPlane{ 10.0f, 40.0f, 100.0f, 255.0f }; // 彩度を上げたディープブルー
	bgColorPlane.ColorBy1();
	BGObject_->objectParts_[0].materialConfig->textureColor = bgColorPlane;
	
	// 模様(Starburst)の設定: 色を少し明るめの暗色に
	BGObject_->objectParts_[1].materialConfig->enableLighting = false;
	Vector4 bgColorStar{ 20.0f, 80.0f, 160.0f, 255.0f }; // 彩度を上げた鮮やかなブルー
	bgColorStar.ColorBy1();
	BGObject_->objectParts_[1].materialConfig->textureColor = bgColorStar;
	
	BGObject_->mainPosition.transform.scale = { 160.0f, 160.0f, 100.0f };

	titleTextObject_ = new Object;

	titleTextModelHandle_ = system_->SetModelObj("GAME/resources/TitleScene/TitleText.obj");
	titleTextObject_->IntObject(system_);
	titleTextObject_->CreateModelData(titleTextModelHandle_);
	titleTextObject_->mainPosition.transform = CreateDefaultTransform();
	// タイトル用カメラ（デフォルト位置）で描画するので元の transform に戻す
	titleTextObject_->mainPosition.transform.translate = { 0.0f, 0.0f, 0.0f };
	titleTextObject_->mainPosition.transform.rotate = { 3.1415f / 2.0f ,0.0f, 0.0f };
	titleTextObject_->mainPosition.transform.scale = { 1.0f, 1.0f, 1.0f };

	dust_ = std::make_unique<DustParticle>(system_);

	nextButton_ = std::make_unique<DetailButton>(system);
	nextButton_->SetButton({ 640.0f, 550.0f }, 400.0f, 80.0f);

	tutorialButton_ = std::make_unique<DetailButton>(system);
	tutorialButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);

	tutorialNextButton_ = std::make_unique<DetailButton>(system);
	tutorialNextButton_->SetButton({ 1180.0f, 680.0f }, 160.0f, 50.0f);

	tutorialPrevButton_ = std::make_unique<DetailButton>(system);
	tutorialPrevButton_->SetButton({ 980.0f, 680.0f }, 160.0f, 50.0f); // 右側に並べる

	tutorialCloseButton_ = std::make_unique<DetailButton>(system);
	tutorialCloseButton_->SetButton({ 1180.0f, 680.0f }, 160.0f, 50.0f);

	// チュートリアル画像の読み込み
	tutorialTex1_ = ResourceManager::GetInstance()->LoadCommonTexture("./GAME/resources/TitleScene/tutorial1.png");
	tutorialTex2_ = ResourceManager::GetInstance()->LoadCommonTexture("./GAME/resources/TitleScene/tutorial2.png");

	darkOverlay_ = std::make_unique<SimpleSprite>();
	darkOverlay_->IntObject(system_);
	darkOverlay_->CreateDefaultData();
	darkOverlay_->objectParts_[0].materialConfig->textureHandle = system_->LoadTexture("kEngine/EngineAssets/TemplateResource/texture/white5x5.png");
	darkOverlay_->mainPosition.transform.scale = { 2000.0f, 2000.0f, 1.0f };
	darkOverlay_->mainPosition.transform.translate = { 0.0f, 0.0f, 0.0f };
	darkOverlay_->objectParts_[0].materialConfig->useModelTexture = false;
	darkOverlay_->objectParts_[0].materialConfig->textureColor = { 0.0f, 0.0f, 0.0f, 0.85f };

	tutorialSprite1_ = std::make_unique<SimpleSprite>();
	tutorialSprite1_->IntObject(system_);
	tutorialSprite1_->CreateDefaultData();
	tutorialSprite1_->objectParts_[0].materialConfig->textureHandle = tutorialTex1_;
	tutorialSprite1_->objectParts_[0].materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 0.99f }; // 透過バケットに入れてZソートさせる
	tutorialSprite1_->mainPosition.transform.scale = { 1.0f, 1.0f, 1.0f };
	auto meta1 = ResourceManager::GetInstance()->GetTextureMetaData(tutorialTex1_);
	tutorialSprite1_->mainPosition.transform.translate = { 640.0f - static_cast<float>(meta1.width) / 2.0f, 360.0f - static_cast<float>(meta1.height) / 2.0f, 0.0f };

	tutorialSprite2_ = std::make_unique<SimpleSprite>();
	tutorialSprite2_->IntObject(system_);
	tutorialSprite2_->CreateDefaultData();
	tutorialSprite2_->objectParts_[0].materialConfig->textureHandle = tutorialTex2_;
	tutorialSprite2_->objectParts_[0].materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 0.99f }; // 透過バケットに入れてZソートさせる
	tutorialSprite2_->mainPosition.transform.scale = { 1.0f, 1.0f, 1.0f };
	auto meta2 = ResourceManager::GetInstance()->GetTextureMetaData(tutorialTex2_);
	tutorialSprite2_->mainPosition.transform.translate = { 640.0f - static_cast<float>(meta2.width) / 2.0f, 360.0f - static_cast<float>(meta2.height) / 2.0f, 0.0f };

	font_.Initialize(system_);

	//===============================
	// 背景NPC演出
	//===============================
	titleNpcPlayer_ = std::make_unique<TravelRunner>(system_);
	titleNpcPlayer_->Initialize(kNpcStartX);

	titleNpcDummyData_ = ModBody::CreateDefaultCustomizeData();
	titleNpcPlayer_->SetCustomizeData(titleNpcDummyData_.get());
	titleNpcPlayer_->LoadCustomizeData();
	titleNpcPlayer_->BuildFeaturesFromCustomizeData();
	titleNpcPlayer_->ApplyCustomizeToMovementParam();

	titleNpcModelHandle_ =
		system_->SetModelObj("GAME/resources/modBody/body/body.obj");

	titleNpcManager_ = std::make_unique<TravelNpcManager>(system_);
	titleNpcManager_->npcModelHandle_ = titleNpcModelHandle_;
	titleNpcManager_->InitializeNpcRunners(
		titleNpcDummyData_.get(), titleNpcPlayer_.get(), kNpcLoopLimitX);

	// 4体に絞る
	if (titleNpcManager_->npcRunners_.size() > 4) {
		titleNpcManager_->npcRunners_.resize(4);
	}

	// 各NPC のキャラ付け（速度差・初期ヘッドスタート・横位置）
	struct NpcInit {
		float timingSkill;
		float headStartTime;  // 初期から何秒走った状態にするか
		float laneX;
		float cooldown;
	};
	const NpcInit initData[4] = {
		{ 1.15f, 3.5f,  0.0f, 3.5f },  // 速い・先頭
		{ 1.05f, 2.5f, -3.0f, 3.2f },  // 追加: やや速い・奥レーン
		{ 1.00f, 1.5f, -1.5f, 2.8f },  // 中・中盤
		{ 0.85f, 0.0f,  1.5f, 2.2f },  // 遅い・手前・スタート地点
	};

	npcLoopSettings_.clear();
	npcLoopSettings_.resize(titleNpcManager_->npcRunners_.size());

	for (size_t i = 0; i < titleNpcManager_->npcRunners_.size() && i < 4; ++i) {
		auto& npc = titleNpcManager_->npcRunners_[i];
		npc.timingSkill    = initData[i].timingSkill;
		npc.headStartSpeed = 2.0f * npc.timingSkill;
		npc.startDelay     = 0.0f;
		npc.started        = true;
		npc.laneX          = initData[i].laneX;
		npcLoopSettings_[i].cooldownDuration = initData[i].cooldown;

		// 最初の1回目のランダムプリセット適用
		ResetTitleNpcBody(static_cast<int>(i));

		// ヘッドスタート分のシミュレーションを回して初期位置をずらす
		if (initData[i].headStartTime > 0.0f) {
			titleNpcManager_->SimulateNpcHeadStart(
				npc, initData[i].headStartTime,
				static_cast<int>(i), kNpcLoopLimitX);
		}

		// シミュレーションでゴール超えた場合の安全ネット
		if (npc.finished || npc.runner->GetMoveX() >= kNpcLoopLimitX) {
			static const float kFallbackPos[4] = { 5.0f, 0.0f, -5.0f, -14.0f };
			npc.runner->Initialize(kFallbackPos[i]);
			npc.finished  = false;
			npc.finishRank = -1;
		}
	}
}

TitleScene::~TitleScene() {
	font_.Cleanup();

	titleNpcManager_.reset();
	titleNpcPlayer_.reset();
	titleNpcDummyData_.reset();
	dust_.reset();

	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);
	system_->DestroyCamera(titleCamera_);
	system_->RemoveLight(light1_);
	delete titleTextObject_;
	delete light1_;
}

void TitleScene::ResetTitleNpcBody(int index) {
	if (index < 0 || index >= static_cast<int>(titleNpcManager_->npcRunners_.size())) {
		return;
	}
	auto& npc = titleNpcManager_->npcRunners_[index];

	// ランダムなプリセットを選択
	std::vector<NpcPresetType> presetPool = {
		NpcPresetType::HeadBig, NpcPresetType::BigTorso,
		NpcPresetType::LongLeg, NpcPresetType::Gorilla, NpcPresetType::Slender,
		NpcPresetType::Chubby, NpcPresetType::Giant, NpcPresetType::Mini,
		NpcPresetType::LongArm, NpcPresetType::WideShoulder, NpcPresetType::WideHip,
		NpcPresetType::MutantAsura, NpcPresetType::OctopusLegs
	};

	// 現在他のNPCが使用中のプリセットを除外する
	std::vector<NpcPresetType> availablePool;
	for (auto preset : presetPool) {
		bool used = false;
		for (size_t i = 0; i < npcLoopSettings_.size(); ++i) {
			if (static_cast<int>(i) != index && npcLoopSettings_[i].currentPresetId == static_cast<int>(preset)) {
				used = true;
				break;
			}
		}
		if (!used) {
			availablePool.push_back(preset);
		}
	}

	if (availablePool.empty()) {
		availablePool = presetPool; // 万が一枯渇した場合は元に戻す
	}

	int randIdx = rand() % availablePool.size();
	NpcPresetType randomPreset = availablePool[randIdx];

	npcLoopSettings_[index].currentPresetId = static_cast<int>(randomPreset);

	// 新しいプリセットデータを作成して適用
	auto newPreset = ModCustomizeDataStore::CreateNpcPreset(randomPreset, nullptr);
	if (newPreset) {
		npc.customizeData = std::move(newPreset);
		npc.runner->SetCustomizeData(npc.customizeData.get());
		npc.runner->LoadCustomizeData();
		npc.runner->BuildFeaturesFromCustomizeData();
		npc.runner->BuildAllVisualParts();
		npc.runner->ApplyCustomizeToMovementParam();
	}
}

void TitleScene::ResetTitleNpc(int index) {
	if (index < 0 || index >= static_cast<int>(titleNpcManager_->npcRunners_.size())) {
		return;
	}
	auto& npc = titleNpcManager_->npcRunners_[index];

	// 位置と状態のリセット
	npc.runner->Initialize(kNpcStartX);

	// 体型をランダムに変更
	ResetTitleNpcBody(index);

	npc.leftInput = false;
	npc.rightInput = false;
	npc.isKickHolding = false;
	npc.kickHoldLeft = false;
	npc.hasKickPlan = false;
	npc.kickedThisAirborne = false;
	npc.prevGrounded = true;

	npc.finished = false;
	npc.finishRank = -1;

	// startDelay をそのままクールタイムとして流用
	npc.started = false;
	npc.startDelay = npcLoopSettings_[index].cooldownDuration;
}

void TitleScene::Update() {
	system_->SetCamera(usingCamera_);
	const float dt = system_->GetDeltaTime();

	// -------------------------------
	// Update Logic
	// -------------------------------
	if (BGObject_) {
		BGObject_->objectParts_[1].transform.rotate.z -= 0.001f;
		BGObject_->Update(titleCamera_);
	}

	// ロゴのステート遷移
	float safeDt = (dt > 0.1f) ? 0.1f : dt;
	if (!isTutorialMode_) {
		logoAnimTimer_ += safeDt;
		logoStateTimer_ += safeDt;
	}
	
	// デバッグ機能：手動で発作を引き起こす
#ifdef _DEBUG
	if (system_->GetTriggerOn(DIK_2)) {
		currentLogoState_ = LogoAnimState::DropStamp;
		logoStateTimer_ = 0.0f;
		logoNextStateDuration_ = 2.5f;
		for(int i=0; i<5; ++i) hasSpawnedDustArray_[i] = false;
	}
	if (system_->GetTriggerOn(DIK_3)) {
		currentLogoState_ = LogoAnimState::Awakening;
		logoStateTimer_ = 0.0f;
		logoNextStateDuration_ = 2.5f;
		hasSpawnedDust_ = false;
	}
	if (system_->GetTriggerOn(DIK_4)) {
		currentLogoState_ = LogoAnimState::SpinJump;
		logoStateTimer_ = 0.0f;
		logoNextStateDuration_ = 2.5f;
	}
#endif

	if (logoStateTimer_ >= logoNextStateDuration_) {
		logoStateTimer_ = 0.0f;
		if (currentLogoState_ == LogoAnimState::Wave) {
			int r = rand() % 3;
			if (r == 0) {
				currentLogoState_ = LogoAnimState::DropStamp;
				for(int i=0; i<5; ++i) hasSpawnedDustArray_[i] = false;
			}
			else if (r == 1) {
				currentLogoState_ = LogoAnimState::Awakening;
				hasSpawnedDust_ = false;
			}
			else currentLogoState_ = LogoAnimState::SpinJump;
			
			logoNextStateDuration_ = 3.0f;
		} else {
			currentLogoState_ = LogoAnimState::Wave;
			// 平常時（ウェーブ）は10秒〜20秒と長くし、発作をレアにする
			logoNextStateDuration_ = 10.0f + (rand() % 100) * 0.1f;
		}
	}

	if (titleTextObject_) {
		// 基本姿勢を一度リセット
		titleTextObject_->mainPosition.transform.translate = { 0.0f, 0.0f, 0.0f };
		titleTextObject_->mainPosition.transform.rotate = { 3.1415f / 2.0f, 0.0f, 0.0f };
		titleTextObject_->mainPosition.transform.scale = { 0.9f, 0.9f, 0.9f };

		for (auto& part : titleTextObject_->objectParts_) {
			int partIndex = static_cast<int>(&part - &titleTextObject_->objectParts_[0]);
			
			// Blender側で全文字を原点(0,0,0)に重ねて出力したため、プログラム側で横に並べる
			// 0.8f の数値を変更すると、文字と文字の隙間（カーニング）を調整できます
			float initialX = (partIndex - 2.0f) * 0.7f;
			
			part.transform.translate = { initialX, 0.0f, 0.0f };
			part.transform.rotate = { 0.0f, 0.0f, 0.0f };
			part.transform.scale = { 1.0f, 1.0f, 1.0f };
			// テクスチャの模様を横に4倍に引き伸ばし、色の幅を広くする
			part.materialConfig->uvScale = { 0.25f, 1.0f, 1.0f };
			
			// 文字の順番(partIndex)に合わせて横にズラし、4文字全体で1つの大きなグラデーションにする
			float baseUvOffset = partIndex * 0.25f;

			// テクスチャの模様が常時ゆっくり流れるようにUVをスクロールさせる
			part.materialConfig->uvTranslate = { baseUvOffset + (logoAnimTimer_ * 0.05f), logoAnimTimer_ * -0.025f, 0.0f };
			part.materialConfig->MakeUVMatrix();
			part.materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 1.0f };

			int waveOrder = partIndex;

			// ベースウェーブの計算
			float baseY = std::sinf(logoAnimTimer_ * 4.0f - waveOrder * 0.6f) * 0.15f;
			float baseRotZ = std::sinf(logoAnimTimer_ * 3.0f - waveOrder * 0.4f) * 0.05f;

			if (currentLogoState_ == LogoAnimState::Wave) {
				part.transform.translate = { initialX, baseY, 0.0f };
				part.transform.rotate = { 0.0f, 0.0f, baseRotZ };
			}
			else if (currentLogoState_ == LogoAnimState::SpinJump) {
				float jumpTime = logoStateTimer_ * 6.0f - waveOrder * 0.5f;
				float jumpY = 0.0f;
				float jumpRot = 0.0f;
				
				if (jumpTime > 0.0f && jumpTime < 3.1415f) {
					jumpY = std::sinf(jumpTime) * 0.4f;
					jumpRot = (jumpTime / 3.1415f) * (3.1415f * 2.0f);
					part.transform.scale = { 1.0f - jumpY * 0.3f, 1.0f + jumpY * 0.6f, 1.0f };
				}
				
				part.transform.translate = { initialX, baseY, -jumpY };
				// 横回転（スピン）
				part.transform.rotate = { 0.0f, 0.0f, baseRotZ + jumpRot };
			}
			else if (currentLogoState_ == LogoAnimState::DropStamp) {
				float time = logoStateTimer_ * 4.0f - waveOrder * 0.6f; 
				float dropZ = 0.0f;
				
				if (time > 0.0f && time < 3.1415f) {
					float progress = time / 3.1415f; 
					if (progress < 0.5f) {
						// ふわっと画面内の上部へ持ち上がる
						float t = progress / 0.5f;
						dropZ = -2.5f * std::sinf(t * 3.1415f / 2.0f); 
					} else if (progress < 0.6f) {
						// 空中で一瞬静止してタメる
						dropZ = -2.5f;
					} else if (progress < 0.7f) {
						// ドスンと一気に急降下
						float t = (progress - 0.6f) / 0.1f;
						dropZ = -2.5f * (1.0f - t);
						
						// 着地の瞬間に個別の砂埃を出す
						if (t >= 0.9f && partIndex >= 0 && partIndex < 5 && !hasSpawnedDustArray_[partIndex]) {
							hasSpawnedDustArray_[partIndex] = true;
							// Y=-0.35f が文字の接地する足元の底面
							if (dust_) dust_->Spawn({ initialX, -0.35f, baseY });
						}
					} else {
						// 着地後のバウンド
						float bounceT = (progress - 0.7f) / 0.3f;
						dropZ = -0.5f * std::sinf(bounceT * 3.1415f * 2.0f) * (1.0f - bounceT);
						if (dropZ > 0.0f) dropZ = 0.0f; 
						
						// フレーム落ちで落下中に出なかった場合は確実に出す
						if (partIndex >= 0 && partIndex < 5 && !hasSpawnedDustArray_[partIndex]) {
							hasSpawnedDustArray_[partIndex] = true;
							if (dust_) dust_->Spawn({ initialX, -0.35f, baseY });
						}
					}
				}
				
				part.transform.translate = { initialX, baseY, dropZ };
				part.transform.rotate = { 0.0f, 0.0f, baseRotZ };
			}
			else if (currentLogoState_ == LogoAnimState::Awakening) {
				float progress = logoStateTimer_ / 2.0f; // 2秒間の演出
				float floatZ = 0.0f;
				float tiltX = 0.0f;
				float shakeX = 0.0f;
				float shakeY = 0.0f;
				
				if (progress >= 0.0f && progress < 1.0f) {
					if (progress < 0.95f) { // 浮上にかける時間をさらに長く
						// ゆっくり少し浮上（画面外に行かないように）
						float t = progress / 0.95f;
						floatZ = -1.5f * std::sinf(t * 3.1415f / 2.0f); // 高さを抑える
						tiltX = 0.0f; 
						
						// 震える
						shakeX = ((rand() % 100) / 100.0f - 0.5f) * 0.15f * t;
						shakeY = ((rand() % 100) / 100.0f - 0.5f) * 0.15f * t;
						
					} else {
						// 0.95〜1.0で一気に元の位置に叩きつける（超高速落下）
						float t = (progress - 0.95f) / 0.05f; // 0~1
						floatZ = -1.5f * (1.0f - t);
						tiltX = 0.0f;
						
						// 着地した瞬間に砂埃を出す（全文字同時に1回だけ）
						if (partIndex == 0 && !hasSpawnedDust_) {
							hasSpawnedDust_ = true;
							for (int i = 0; i < 5; ++i) {
								float x = (i - 2.0f) * 0.7f; // 各文字の位置
								// 足元のY座標は -0.35f 付近
								if (dust_) dust_->Spawn({ x, -0.35f, baseY }); 
							}
						}
					}
				} else {
					part.materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 1.0f };
					
					// フレーム落ちで落下中に出なかった場合は確実に出す
					if (partIndex == 0 && !hasSpawnedDust_) {
						hasSpawnedDust_ = true;
						for (int i = 0; i < 5; ++i) {
							float x = (i - 2.0f) * 0.7f; 
							if (dust_) dust_->Spawn({ x, -0.35f, baseY }); 
						}
					}
				}
				part.transform.translate = { initialX + shakeX, baseY + shakeY, floatZ };
				part.transform.rotate = { tiltX, 0.0f, baseRotZ };
			}

			partIndex++;
		}
	}

	if (isTutorialMode_) {
		const float kAnimTime = 0.25f;
		
		// 背景用タイマー
		if (isTutorialClosing_) {
			tutorialAnimTimer_ -= system_->GetDeltaTime();
			if (tutorialAnimTimer_ <= 0.0f) {
				tutorialAnimTimer_ = 0.0f;
				isTutorialMode_ = false;
				isTutorialClosing_ = false;
			}
		} else {
			tutorialAnimTimer_ += system_->GetDeltaTime();
			if (tutorialAnimTimer_ > kAnimTime) {
				tutorialAnimTimer_ = kAnimTime;
			}
		}

		// 画像の切り替え演出 (スケールアニメーション)
		float imgT = 0.0f;
		if (isTutorialSwitching_) {
			// 切り替え時は一度縮んでから次の画像になって膨らむ
			tutorialImageAnimTimer_ += dt * 3.0f;
			if (tutorialImageAnimTimer_ >= 2.0f) {
				isTutorialSwitching_ = false;
				tutorialImageAnimTimer_ = 1.0f;
			}
			
			if (tutorialImageAnimTimer_ < 1.0f) {
				// 前の画像が縮む (1.0 -> 0.0)
				imgT = 1.0f - tutorialImageAnimTimer_;
			} else {
				// 新しい画像が膨らむ (0.0 -> 1.0)
				imgT = tutorialImageAnimTimer_ - 1.0f;
				if (tutorialPage_ == 0) tutorialPage_ = 1; // ここで実際のページ切り替え
			}
		} else {
			if (tutorialAnimTimer_ > 1.0f) {
				imgT = std::clamp(tutorialAnimTimer_ - 1.0f, 0.0f, 1.0f);
			} else {
				imgT = tutorialAnimTimer_ / kAnimTime;
			}
		}

		// ポップアップのイージング (EaseOutBack相当)
		float scale = 0.0f;
		if (imgT > 0.0f) {
			const float c1 = 1.70158f;
			const float c3 = c1 + 1.0f;
			scale = 1.0f + c3 * std::powf(imgT - 1.0f, 3.0f) + c1 * std::powf(imgT - 1.0f, 2.0f);
		}

		tutorialSprite1_->mainPosition.transform.scale = { scale, scale, 1.0f };
		tutorialSprite2_->mainPosition.transform.scale = { scale, scale, 1.0f };

		auto meta1 = ResourceManager::GetInstance()->GetTextureMetaData(tutorialTex1_);
		tutorialSprite1_->mainPosition.transform.translate = { 640.0f - static_cast<float>(meta1.width) * scale / 2.0f, 360.0f - static_cast<float>(meta1.height) * scale / 2.0f, 0.0f };

		auto meta2 = ResourceManager::GetInstance()->GetTextureMetaData(tutorialTex2_);
		tutorialSprite2_->mainPosition.transform.translate = { 640.0f - static_cast<float>(meta2.width) * scale / 2.0f, 360.0f - static_cast<float>(meta2.height) * scale / 2.0f, 0.0f };

		// 背景のフェードイン
		darkOverlay_->objectParts_[0].materialConfig->textureColor.w = 0.85f * (tutorialAnimTimer_ / kAnimTime);

		darkOverlay_->Update(nullptr);
		tutorialSprite1_->Update(nullptr);
		tutorialSprite2_->Update(nullptr);

		// マウスクリックかスペースキーでページ送り (アニメーション完了後のみ受付)
		if (!fade_.IsBusy() && imgT >= 1.0f && !isTutorialClosing_ && !isTutorialSwitching_) {
			if (tutorialPage_ == 0) {
				tutorialNextButton_->Update();
				if (system_->GetTriggerOn(DIK_SPACE) || tutorialNextButton_->GetIsPress()) {
					isTutorialSwitching_ = true;
					tutorialImageAnimTimer_ = 0.0f;
				}
			} else {
				tutorialPrevButton_->Update();
				tutorialCloseButton_->Update();
				if (tutorialPrevButton_->GetIsPress()) {
					tutorialPage_ = 0;
					tutorialImageAnimTimer_ = 0.0f;
					isTutorialSwitching_ = false;
				} else if (system_->GetTriggerOn(DIK_SPACE) || tutorialCloseButton_->GetIsPress()) {
					isTutorialClosing_ = true; // チュートリアル終了へ
				}
			}
		}
		return;
	} else {
		if (!fade_.IsBusy()) {
			nextButton_->Update();
			if (tutorialButton_) {
				tutorialButton_->Update();
			
			if (tutorialButton_->GetIsPress()) {
				isTutorialMode_ = true;
				isTutorialClosing_ = false;
				isTutorialSwitching_ = false;
				tutorialAnimTimer_ = 0.0f;
				tutorialImageAnimTimer_ = 0.0f;
				tutorialPage_ = 0;
			}
		}
		}
	}

	//===============================
	// 背景NPC更新（ループ管理）
	//===============================
	if (titleNpcManager_) {
		titleNpcManager_->UpdateNpcRunners(dt, kNpcLoopLimitX, usingCamera_);

		for (int i = 0; i < static_cast<int>(titleNpcManager_->npcRunners_.size()); ++i) {
			if (titleNpcManager_->npcRunners_[i].finished) {
				ResetTitleNpc(i);
			}
		}
	}

	// スペースキーでお題発表シーンへ
	if (!isTutorialMode_ && !fade_.IsBusy() && (system_->GetTriggerOn(DIK_SPACE) || nextButton_->GetIsPress())) {
		fade_.StartFadeOut();
		isStartTransition_ = true;
	}

	// フェード更新
	fade_.Update(usingCamera_);

	// フェード終了後にシーン移行
	if (isStartTransition_ && fade_.IsFinished()) {
		outcome_ = SceneOutcome::NEXT;
	}
}

void TitleScene::Draw() {
	if (BGObject_) {
		system_->SetCamera(titleCamera_);
		BGObject_->Draw();
		system_->SetCamera(usingCamera_);
	}

	// 背景NPC（先に描画）
	if (titleNpcManager_) {
		titleNpcManager_->DrawNpcs(kNpcLoopLimitX, true, usingCamera_);
	}

	if (titleTextObject_) {
		// タイトルロゴはデフォルト正面カメラで描画
		system_->SetCamera(titleCamera_);
		titleTextObject_->Update(titleCamera_);
		if (dust_) dust_->Update(titleCamera_);
		
		titleTextObject_->Draw();
		if (dust_) dust_->Draw();
		
		system_->SetCamera(usingCamera_); // NPC カメラに戻す
	}

	nextButton_->Render();
	font_.RenderText(
		"進化しろ！",
		{ 640.0f, 520.0f }, 48.0f,
		BitmapFont::Align::Center, 5, { 1.0f,1.0f,0.0f,1.0f });

	tutorialButton_->Render();
	font_.RenderText(
		"チュートリアル",
		{ 640.0f, 620.0f }, 48.0f,
		BitmapFont::Align::Center, 5, { 1.0f,1.0f,1.0f,1.0f });

	if (isTutorialMode_) {
		darkOverlay_->Draw();
		if (tutorialPage_ == 0) tutorialSprite1_->Draw();
		else if (tutorialPage_ == 1) tutorialSprite2_->Draw();

		// アニメーション完了後のみボタンを表示
		const float kAnimTime = 0.25f;
		float imgT = 1.0f;
		if (tutorialPage_ == 0 && !isTutorialSwitching_) {
			imgT = tutorialAnimTimer_ / kAnimTime;
		} else if (isTutorialClosing_) {
			imgT = 1.0f - (tutorialAnimTimer_ / kAnimTime);
		} else if (isTutorialSwitching_) {
			imgT = tutorialImageAnimTimer_ / kAnimTime;
		}

		if (imgT >= 1.0f && !isTutorialClosing_ && !isTutorialSwitching_) {
			if (tutorialPage_ == 0) {
				tutorialNextButton_->Render();
				font_.RenderText("次へ", { 1180.0f, 665.0f }, 28.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 1.0f, 1.0f });
			} else {
				tutorialPrevButton_->Render();
				font_.RenderText("前へ", { 980.0f, 665.0f }, 28.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 1.0f, 1.0f });
				
				tutorialCloseButton_->Render();
				font_.RenderText("とじる", { 1180.0f, 665.0f }, 28.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
	}

#ifdef USE_IMGUI
	// 現在シーン表示
	ImGui::Begin("Scene");
	ImGui::Text("TitleScene");
	ImGui::End();

	ImGui::Begin("Title Scene Debug");
	const char* presetNames[] = {
		"Default", "HeadBig", "LongLeg", "BigTorso", "Gorilla", "Slender", "Chubby", "Giant", "Mini", "LongArm", "WideShoulder", "WideHip", "MutantAsura", "OctopusLegs"
	};
	static int currentPresetIndex = static_cast<int>(NpcPresetType::OctopusLegs);
	ImGui::Combo("Force Preset", &currentPresetIndex, presetNames, IM_ARRAYSIZE(presetNames));
	if (ImGui::Button("Apply Preset to All NPCs")) {
		for (size_t i = 0; i < titleNpcManager_->npcRunners_.size(); ++i) {
			auto& npc = titleNpcManager_->npcRunners_[i];
			npcLoopSettings_[i].currentPresetId = currentPresetIndex;
			auto presetData = ModCustomizeDataStore::CreateNpcPreset(
				static_cast<NpcPresetType>(currentPresetIndex), titleNpcDummyData_.get());
			npc.customizeData = std::move(presetData);
			npc.runner->SetCustomizeData(npc.customizeData.get());
			npc.runner->LoadCustomizeData();
			npc.runner->BuildFeaturesFromCustomizeData();
			npc.runner->BuildAllVisualParts();
			npc.runner->ApplyCustomizeToMovementParam();
		}
	}
	ImGui::End();
#endif

	// フェード描画
	fade_.Draw();
}