#include "TitleScene.h"
#include "TravelScene.h"
#include "ModScene.h"
#include "GAME/actor/ModCustomizeDataStore.h"
#include <random>

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

	titleTextObject_ = new Object;

	titleTextModelHandle_ = system_->SetModelObj("GAME/resources/TitleScene/TitleText.obj");
	titleTextObject_->IntObject(system_);
	titleTextObject_->CreateModelData(titleTextModelHandle_);
	titleTextObject_->mainPosition.transform = CreateDefaultTransform();
	// タイトル用カメラ（デフォルト位置）で描画するので元の transform に戻す
	titleTextObject_->mainPosition.transform.translate = { 0.0f, 0.0f, 0.0f };
	titleTextObject_->mainPosition.transform.rotate = { 3.1415f / 2.0f ,0.0f, 0.0f };
	titleTextObject_->mainPosition.transform.scale = { 1.0f,1.0f,1.0f };

	nextButton_ = std::make_unique<DetailButton>(system);
	nextButton_->SetButton({ 640.0f, 550.0f }, 400.0f, 80.0f);

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

	// 3体に絞る
	if (titleNpcManager_->npcRunners_.size() > 3) {
		titleNpcManager_->npcRunners_.resize(3);
	}

	// 各NPC のキャラ付け（速度差・初期ヘッドスタート・横位置）
	struct NpcInit {
		float timingSkill;
		float headStartTime;  // 初期から何秒走った状態にするか
		float laneX;
		float cooldown;
	};
	const NpcInit initData[3] = {
		{ 1.15f, 3.5f,  0.0f, 3.5f },  // 速い・先頭
		{ 1.00f, 1.5f, -1.5f, 2.8f },  // 中・中盤
		{ 0.85f, 0.0f,  1.5f, 2.2f },  // 遅い・スタート地点
	};

	npcLoopSettings_.clear();
	npcLoopSettings_.resize(titleNpcManager_->npcRunners_.size());

	for (size_t i = 0; i < titleNpcManager_->npcRunners_.size() && i < 3; ++i) {
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
			static const float kFallbackPos[3] = { 5.0f, -5.0f, -14.0f };
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
		NpcPresetType::LowHead, NpcPresetType::Default
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

	nextButton_->Update();

	//===============================
	// 背景NPC更新（ループ管理）
	//===============================
	const float dt = system_->GetDeltaTime();
	if (titleNpcManager_) {
		titleNpcManager_->UpdateNpcRunners(dt, kNpcLoopLimitX, usingCamera_);

		for (int i = 0; i < static_cast<int>(titleNpcManager_->npcRunners_.size()); ++i) {
			if (titleNpcManager_->npcRunners_[i].finished) {
				ResetTitleNpc(i);
			}
		}
	}

	// スペースキーでお題発表シーンへ
	if (!fade_.IsBusy() && (system_->GetTriggerOn(DIK_SPACE) || nextButton_->GetIsPress())) {
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

	// 背景NPC（先に描画）
	if (titleNpcManager_) {
		titleNpcManager_->DrawNpcs(kNpcLoopLimitX, true, usingCamera_);
	}

	if (titleTextObject_) {
		// タイトルロゴはデフォルト正面カメラで描画
		system_->SetCamera(titleCamera_);
		titleTextObject_->Update(titleCamera_);
		titleTextObject_->Draw();
		system_->SetCamera(usingCamera_); // NPC カメラに戻す
	}

	nextButton_->Render();

	font_.RenderText(
		"進化する",
		{ 640.0f, 520.0f }, 48.0f,
		BitmapFont::Align::Center, 5, { 1.0f,1.0f,0.0f,1.0f });

#ifdef USE_IMGUI
	// 現在シーン表示
	ImGui::Begin("Scene");
	ImGui::Text("TitleScene");
	ImGui::End();
#endif

	// フェード描画
	fade_.Draw();
}